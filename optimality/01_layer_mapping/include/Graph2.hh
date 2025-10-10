#pragma once
/**
 * Graph.hh — Minimal waypoint/visibility graph for multi-robot planning
 *
 * Design goals:
 *  - Sparse node set (charging points, AFK/entry points, pickup/drop points, obstacle corners, key waypoints)
 *  - Undirected edges with weight = speed limit on that segment
 *  - Geometry-aware (each node has (x,y); each edge stores Euclidean length)
 *  - Convenience to compute traversal time = length / speed
 *  - Reservation API to prevent head-on collisions: two robots may NOT traverse the same edge
 *    in opposite directions with overlapping time windows. Same-direction sharing is allowed
 *    (optionally with a configurable headway). Node occupancy can also be protected.
 *
 * Notes:
 *  - This header provides a complete interface and inline implementations for lightweight use.
 *  - Heavier geometry checks (e.g., line-of-sight / obstacle intersection tests) should live in Graph.cc.
 *  - Time is expressed in seconds, positions in meters, speeds in meters/second.
 */

#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace warehouse {

class Graph {
public:
    // -------- Public type aliases
    using NodeId  = int32_t;
    using EdgeId  = int32_t;
    using RobotId = int32_t;

    // -------- Node categories (optional metadata)
    enum class NodeType : uint8_t {
        Waypoint = 0,
        Charging,
        AFK,
        Pickup,
        Dropoff,
        ObstacleCorner
    };

    struct Node {
        NodeId   id{ -1 };
        double   x{ 0.0 };
        double   y{ 0.0 };
        NodeType type{ NodeType::Waypoint };
        std::string label; // optional human-readable tag
    };

    // Undirected edge between (u,v). Weight = speed (m/s). length is Euclidean (m).
    struct Edge {
        EdgeId  id{ -1 };
        NodeId  u{ -1 };
        NodeId  v{ -1 };
        double  speed{ 0.0 };     // edge weight as requested
        double  length{ 0.0 };    // cached Euclidean length (meters)
        bool    one_lane{ true }; // if true, prevent opposite-direction overlap
        std::string label;        // optional tag (e.g., "main corridor", "slow zone")
    };

    // Adjacency entry (undirected graph; we store both directions in adjacency list)
    struct Adjacent {
        NodeId to{ -1 };
        EdgeId edge{ -1 };
    };

    // -------- Construction
    Graph() = default;

    // Clear all graph data (nodes/edges/adjacency and reservations)
    void Clear() {
        nodes_.clear();
        edges_.clear();
        adjacency_.clear();
        next_node_id_ = 0;
        next_edge_id_ = 0;
        ClearReservations();
    }

    // -------- Node API
    NodeId AddNode(double x, double y, NodeType type = NodeType::Waypoint, std::string label = {}) {
        NodeId id = next_node_id_++;
        nodes_.push_back(Node{ id, x, y, type, std::move(label) });
        adjacency_.emplace_back(); // adjacency for this node
        return id;
    }

    const Node& GetNode(NodeId id) const {
        if (id < 0 || id >= static_cast<NodeId>(nodes_.size())) throw std::out_of_range("GetNode: invalid id");
        return nodes_[id];
    }

    std::vector<NodeId> Nodes() const {
        std::vector<NodeId> ids;
        ids.reserve(nodes_.size());
        for (const auto& n : nodes_) ids.push_back(n.id);
        return ids;
    }

    size_t NodeCount() const { return nodes_.size(); }

    // -------- Edge API
    // Adds an UNDIRECTED edge (u<->v) with speed limit (m/s). Automatically computes length.
    // Throws if u==v or ids invalid or speed<=0.
    EdgeId AddEdge(NodeId u, NodeId v, double speed_mps, bool one_lane = true, std::string label = {}) {
        ValidateNodeId(u);
        ValidateNodeId(v);
        if (u == v) throw std::invalid_argument("AddEdge: u==v");
        if (!(speed_mps > 0.0)) throw std::invalid_argument("AddEdge: speed must be > 0");

        const auto& nu = nodes_[u];
        const auto& nv = nodes_[v];
        const double len = std::hypot(nu.x - nv.x, nu.y - nv.y);

        EdgeId id = next_edge_id_++;
        edges_.push_back(Edge{ id, u, v, speed_mps, len, one_lane, std::move(label) });

        // undirected adjacency
        adjacency_[u].push_back(Adjacent{ v, id });
        adjacency_[v].push_back(Adjacent{ u, id });

        return id;
    }

    const Edge& GetEdge(EdgeId id) const {
        if (id < 0 || id >= static_cast<EdgeId>(edges_.size())) throw std::out_of_range("GetEdge: invalid id");
        return edges_[id];
    }

    // Returns adjacency list for a node (neighbors and edge ids)
    const std::vector<Adjacent>& Neighbors(NodeId id) const {
        ValidateNodeId(id);
        return adjacency_[id];
    }

    size_t EdgeCount() const { return edges_.size(); }

    // Convenience: traversal time for an edge = length / speed (seconds)
    double EdgeTraversalTime(EdgeId id) const {
        const auto& e = GetEdge(id);
        return e.length / e.speed;
    }

    // Convenience: Euclidean distance between two nodes
    double Distance(NodeId a, NodeId b) const {
        ValidateNodeId(a); ValidateNodeId(b);
        const auto& na = nodes_[a];
        const auto& nb = nodes_[b];
        return std::hypot(na.x - nb.x, na.y - nb.y);
    }

    // -------- Reservation system (collision avoidance)
    //
    // Prevents head-on conflicts:
    //   - Two robots may NOT traverse the same one-lane edge in opposite directions with overlapping time windows.
    //   - Same-direction sharing is allowed; a configurable headway can be applied if desired.
    //   - Optional node occupancy protection (prevent two robots from occupying same node in the same instant).
    //
    // We use continuous-time intervals [t_enter, t_exit). Time units: seconds.

    struct EdgeReservation {
        EdgeId  edge{ -1 };
        RobotId robot{ -1 };
        // dir = +1 if traversal is u->v, -1 if v->u
        int     dir{ +1 };
        double  t_enter{ 0.0 };
        double  t_exit{ 0.0 };
    };

    struct NodeReservation {
        NodeId  node{ -1 };
        RobotId robot{ -1 };
        double  t_enter{ 0.0 };
        double  t_exit{ 0.0 };
    };

    struct ReservationParams {
        // Minimum separation between two traversals in the SAME direction on the same edge (seconds)
        double same_dir_headway_s   = 0.0;
        // Minimum separation between two traversals in OPPOSITE directions on the same edge (seconds)
        double opp_dir_headway_s    = 0.0;
        // Minimum separation when occupying the same node (seconds)
        double node_headway_s       = 0.0;
    };

    void SetReservationParams(const ReservationParams& p) { params_ = p; }
    const ReservationParams& GetReservationParams() const { return params_; }

    // Compute entry/exit times for traversing an edge starting at t_enter.
    // Direction is inferred from (from -> to); throws if (from,to) not matching edge endpoints.
    inline std::pair<double,double> EdgeInterval(EdgeId e, NodeId from, NodeId to, double t_enter) const {
        const auto& E = GetEdge(e);
        const int dir = Direction(E, from, to);
        (void)dir; // for clarity; direction only used for validation
        const double dt = EdgeTraversalTime(e);
        return { t_enter, t_enter + dt };
    }

    // Check if an edge traversal is admissible given current reservations.
    // Allows:
    //   - Same-direction overlaps (with same_dir_headway_s separation if >0)
    // Disallows:
    //   - Opposite-direction overlaps on one-lane edges, honoring opp_dir_headway_s.
    bool CanTraverseEdge(EdgeId e, NodeId from, NodeId to, double t_enter, double t_exit, RobotId robot) const {
        const auto& E = GetEdge(e);
        const int dir = Direction(E, from, to);

        // Check edge reservations
        auto it = edge_res_.find(e);
        if (it != edge_res_.end()) {
            for (const auto& r : it->second) {
                if (r.robot == robot) continue; // ignore self
                const bool opposite = (r.dir != dir);
                const double sep    = opposite ? params_.opp_dir_headway_s : params_.same_dir_headway_s;
                if (!intervals_separated(r.t_enter, r.t_exit, t_enter, t_exit, sep)) {
                    // If edge is not one-lane, we only block if opposite & one_lane
                    if (opposite && E.one_lane) return false;
                    if (!opposite) {
                        // same direction: only block if headway required and violated
                        if (params_.same_dir_headway_s > 0.0) return false;
                    }
                }
            }
        }
        return true;
    }

    // Reserve an edge traversal if allowed; returns true on success and records reservation.
    bool ReserveEdge(EdgeId e, NodeId from, NodeId to, double t_enter, double t_exit, RobotId robot) {
        if (!CanTraverseEdge(e, from, to, t_enter, t_exit, robot)) return false;
        const int dir = Direction(GetEdge(e), from, to);
        edge_res_[e].push_back(EdgeReservation{ e, robot, dir, t_enter, t_exit });
        return true;
    }

    // Node occupancy checks (optional). If node_headway_s == 0, this is effectively off.
    bool CanOccupyNode(NodeId n, double t_enter, double t_exit, RobotId robot) const {
        if (params_.node_headway_s <= 0.0) return true;
        auto it = node_res_.find(n);
        if (it == node_res_.end()) return true;
        for (const auto& r : it->second) {
            if (r.robot == robot) continue;
            if (!intervals_separated(r.t_enter, r.t_exit, t_enter, t_exit, params_.node_headway_s)) {
                return false;
            }
        }
        return true;
    }

    bool ReserveNode(NodeId n, double t_enter, double t_exit, RobotId robot) {
        if (!CanOccupyNode(n, t_enter, t_exit, robot)) return false;
        node_res_[n].push_back(NodeReservation{ n, robot, t_enter, t_exit });
        return true;
    }

    // Remove all reservations for a given robot (e.g., replan)
    void ClearReservationsForRobot(RobotId r) {
        for (auto& kv : edge_res_) {
            auto& vec = kv.second;
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                                     [r](const EdgeReservation& er){ return er.robot == r; }),
                      vec.end());
        }
        for (auto& kv : node_res_) {
            auto& vec = kv.second;
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                                     [r](const NodeReservation& nr){ return nr.robot == r; }),
                      vec.end());
        }
    }

    // Remove all reservations (edges & nodes)
    void ClearReservations() {
        edge_res_.clear();
        node_res_.clear();
    }

    // -------- Utilities

    // Returns +1 if (from->to) equals (u->v), -1 if equals (v->u); throws if not on this edge
    static int Direction(const Edge& e, NodeId from, NodeId to) {
        if (e.u == from && e.v == to) return +1;
        if (e.v == from && e.u == to) return -1;
        throw std::invalid_argument("Direction: (from,to) is not this edge");
    }

    // Check if straight-line connection between two nodes is clear (to be implemented in .cc if needed)
    // Here declared for completeness; default inline returns true.
    bool HasLineOfSight(NodeId /*a*/, NodeId /*b*/) const {
        // Implement ray casting / grid checking in Graph.cc against your occupancy map
        return true;
    }

private:
    // -------- Internal data
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<std::vector<Adjacent>> adjacency_;
    NodeId next_node_id_{ 0 };
    EdgeId next_edge_id_{ 0 };

    // Reservations
    ReservationParams params_{};
    std::unordered_map<EdgeId, std::vector<EdgeReservation>> edge_res_;
    std::unordered_map<NodeId,  std::vector<NodeReservation>> node_res_;

    // -------- Helpers
    void ValidateNodeId(NodeId id) const {
        if (id < 0 || id >= static_cast<NodeId>(nodes_.size()))
            throw std::out_of_range("Invalid node id");
    }

    // Returns true if intervals [a0,a1) and [b0,b1) are separated by >= sep seconds
    static bool intervals_separated(double a0, double a1, double b0, double b1, double sep) {
        // Extend both intervals by sep/2 on each side (equivalently, require |gap| >= sep)
        const double A0 = a0 - sep * 0.5;
        const double A1 = a1 + sep * 0.5;
        const double B0 = b0 - sep * 0.5;
        const double B1 = b1 + sep * 0.5;
        return (A1 <= B0) || (B1 <= A0);
    }
};

} // namespace warehouse
