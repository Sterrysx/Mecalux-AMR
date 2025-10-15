import { useState, useEffect, useRef, useCallback } from 'react';

interface Task {
  id: string;
  taskId: number;
  fromNode: number | null;
  fromCoords: { x: number; y: number } | null;
  toNode: number | null;
  toCoords: { x: number; y: number } | null;
  travelTime: number;
  executionTime: number;
  cumulativeTime: number;
  batteryLevel: number;
  startTime: number;
  duration: number;
  type?: 'task';
}

interface ChargingEvent {
  type: 'charging';
  batteryBefore: number;
  batteryAfter: number;
  travelTime: number;
  chargingTime: number;
  startTime: number;
  duration: number;
  decision?: {
    currentBattery: number;
    nextTaskId: number;
    nextTaskConsumption: number;
    batteryAfterTask: number;
    threshold: number;
  };
}

type RobotEvent = Task | ChargingEvent;

interface Robot {
  id: number;
  tasks: RobotEvent[];
  initialBattery: number;
  finalBattery: number;
  completionTime: number;
}

interface TimelineVisualizerProps {
  robots: Robot[];
  makespan: number;
  onClose: () => void;
}

export default function TimelineVisualizer({ robots, makespan, onClose }: TimelineVisualizerProps) {
  const [currentTime, setCurrentTime] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [playbackSpeed, setPlaybackSpeed] = useState(1);
  const [autoScroll, setAutoScroll] = useState(true);
  const animationRef = useRef<number | undefined>(undefined);
  const lastUpdateRef = useRef<number>(Date.now());

  // Calculate which events are currently active or completed
  const getEventStatus = (event: RobotEvent) => {
    if (event.type === 'charging') {
      // For charging events, check against charging time only (not travel)
      const chargingStartTime = event.startTime + event.travelTime;
      const chargingEndTime = chargingStartTime + event.chargingTime;
      
      if (currentTime < chargingStartTime) return 'pending';
      if (currentTime >= chargingEndTime) return 'completed';
      return 'active';
    }
    
    // For regular tasks
    const endTime = event.startTime + event.duration;
    if (currentTime < event.startTime) return 'pending';
    if (currentTime >= endTime) return 'completed';
    return 'active';
  };

  // Get progress percentage for active events
  const getEventProgress = (event: RobotEvent) => {
    if (event.type === 'charging') {
      // For charging events, calculate progress based on charging time only (not travel)
      const chargingStartTime = event.startTime + event.travelTime;
      const chargingEndTime = chargingStartTime + event.chargingTime;
      
      if (currentTime < chargingStartTime) return 0;
      if (currentTime >= chargingEndTime) return 100;
      return ((currentTime - chargingStartTime) / event.chargingTime) * 100;
    }
    
    // For regular tasks
    const endTime = event.startTime + event.duration;
    if (currentTime < event.startTime) return 0;
    if (currentTime >= endTime) return 100;
    return ((currentTime - event.startTime) / event.duration) * 100;
  };

  // Animation loop
  useEffect(() => {
    if (!isPlaying) {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
      return;
    }

    const animate = () => {
      const now = Date.now();
      const deltaTime = (now - lastUpdateRef.current) / 1000; // Convert to seconds
      lastUpdateRef.current = now;

      setCurrentTime(prev => {
        const next = prev + deltaTime * playbackSpeed;
        if (next >= makespan) {
          setIsPlaying(false);
          return makespan;
        }
        return next;
      });

      animationRef.current = requestAnimationFrame(animate);
    };

    lastUpdateRef.current = Date.now();
    animationRef.current = requestAnimationFrame(animate);

    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, [isPlaying, playbackSpeed, makespan]);

  // Auto-scroll effect to follow current time
  useEffect(() => {
    if (!autoScroll || !isPlaying) return;

    robots.forEach(robot => {
      const scrollContainer = document.getElementById(`timeline-scroll-${robot.id}`);
      if (scrollContainer) {
        const currentTimePx = currentTime * 10;
        const containerWidth = scrollContainer.clientWidth;
        const scrollLeft = scrollContainer.scrollLeft;
        
        // Keep current time position in the center of the viewport
        const targetScroll = currentTimePx - containerWidth / 2;
        
        // Instant scroll to keep current time visible
        if (targetScroll > scrollLeft || targetScroll < scrollLeft - containerWidth / 4) {
          scrollContainer.scrollLeft = Math.max(0, targetScroll);
        }
      }
    });
  }, [currentTime, autoScroll, isPlaying, robots]);

  // Set up arrow button event listeners
  useEffect(() => {
    const scrollStates: { [key: string]: { isScrolling: boolean; direction: number; animationId?: number; timeoutId?: number } } = {};

    robots.forEach(robot => {
      const leftButton = document.getElementById(`left-arrow-${robot.id}`);
      const rightButton = document.getElementById(`right-arrow-${robot.id}`);
      const scrollContainer = document.getElementById(`timeline-scroll-${robot.id}`);

      if (leftButton && scrollContainer) {
        const handleLeftDown = (e: Event) => {
          e.preventDefault();
          setAutoScroll(false);
          
          const key = `left-${robot.id}`;
          
          // Wait 200ms - if still pressed, start continuous scroll
          scrollStates[key] = { isScrolling: false, direction: -1 };
          scrollStates[key].timeoutId = window.setTimeout(() => {
            scrollStates[key].isScrolling = true;
            
            const scroll = () => {
              if (scrollStates[key]?.isScrolling) {
                scrollContainer.scrollLeft += scrollStates[key].direction * 3;
                scrollStates[key].animationId = requestAnimationFrame(scroll);
              }
            };
            
            scroll();
          }, 200);
        };

        const handleLeftUp = () => {
          const key = `left-${robot.id}`;
          if (scrollStates[key]) {
            const wasHolding = scrollStates[key].isScrolling;
            
            // Clear timeout and animation
            if (scrollStates[key].timeoutId) {
              clearTimeout(scrollStates[key].timeoutId);
            }
            if (scrollStates[key].animationId) {
              cancelAnimationFrame(scrollStates[key].animationId!);
            }
            
            // If it was just a click (not holding), do smooth scroll
            if (!wasHolding) {
              scrollContainer.scrollBy({ left: -200, behavior: 'smooth' });
            } else {
              // If holding, add a smooth deceleration
              let velocity = scrollStates[key].direction * 3;
              const decelerate = () => {
                velocity *= 0.9; // Deceleration factor
                if (Math.abs(velocity) > 0.1) {
                  scrollContainer.scrollLeft += velocity;
                  requestAnimationFrame(decelerate);
                }
              };
              decelerate();
            }
            
            delete scrollStates[key];
          }
        };

        leftButton.addEventListener('pointerdown', handleLeftDown);
        leftButton.addEventListener('pointerup', handleLeftUp);
        leftButton.addEventListener('pointerleave', handleLeftUp);
        leftButton.addEventListener('pointercancel', handleLeftUp);
      }

      if (rightButton && scrollContainer) {
        const handleRightDown = (e: Event) => {
          e.preventDefault();
          setAutoScroll(false);
          
          const key = `right-${robot.id}`;
          
          // Wait 200ms - if still pressed, start continuous scroll
          scrollStates[key] = { isScrolling: false, direction: 1 };
          scrollStates[key].timeoutId = window.setTimeout(() => {
            scrollStates[key].isScrolling = true;
            
            const scroll = () => {
              if (scrollStates[key]?.isScrolling) {
                scrollContainer.scrollLeft += scrollStates[key].direction * 3;
                scrollStates[key].animationId = requestAnimationFrame(scroll);
              }
            };
            
            scroll();
          }, 200);
        };

        const handleRightUp = () => {
          const key = `right-${robot.id}`;
          if (scrollStates[key]) {
            const wasHolding = scrollStates[key].isScrolling;
            
            // Clear timeout and animation
            if (scrollStates[key].timeoutId) {
              clearTimeout(scrollStates[key].timeoutId);
            }
            if (scrollStates[key].animationId) {
              cancelAnimationFrame(scrollStates[key].animationId!);
            }
            
            // If it was just a click (not holding), do smooth scroll
            if (!wasHolding) {
              scrollContainer.scrollBy({ left: 200, behavior: 'smooth' });
            } else {
              // If holding, add a smooth deceleration
              let velocity = scrollStates[key].direction * 3;
              const decelerate = () => {
                velocity *= 0.9; // Deceleration factor
                if (Math.abs(velocity) > 0.1) {
                  scrollContainer.scrollLeft += velocity;
                  requestAnimationFrame(decelerate);
                }
              };
              decelerate();
            }
            
            delete scrollStates[key];
          }
        };

        rightButton.addEventListener('pointerdown', handleRightDown);
        rightButton.addEventListener('pointerup', handleRightUp);
        rightButton.addEventListener('pointerleave', handleRightUp);
        rightButton.addEventListener('pointercancel', handleRightUp);
      }
    });

    // Cleanup
    return () => {
      Object.values(scrollStates).forEach(state => {
        state.isScrolling = false;
        if (state.timeoutId) {
          clearTimeout(state.timeoutId);
        }
        if (state.animationId) {
          cancelAnimationFrame(state.animationId);
        }
      });
    };
  }, [robots]);

  const handlePlayPause = () => {
    setIsPlaying(!isPlaying);
  };

  const handleReset = () => {
    setCurrentTime(0);
    setIsPlaying(false);
  };

  const handleSeek = (e: React.ChangeEvent<HTMLInputElement>) => {
    const value = parseFloat(e.target.value);
    setCurrentTime(value);
  };

  const handleSpeedChange = (speed: number) => {
    setPlaybackSpeed(speed);
  };

  // Get completed events for each robot
  const getCompletedEvents = (robotId: number) => {
    const robot = robots.find(r => r.id === robotId);
    if (!robot) return [];
    return robot.tasks.filter(event => getEventStatus(event) === 'completed');
  };

  return (
    <div className="fixed inset-0 bg-slate-900 bg-opacity-50 z-50 flex items-center justify-center p-4">
      <div className="bg-white rounded-lg shadow-2xl w-full max-w-7xl max-h-[95vh] overflow-hidden flex flex-col">
        {/* Header */}
        <div className="bg-gradient-to-r from-indigo-600 to-purple-600 text-white p-6 flex items-center justify-between">
          <div>
            <h2 className="text-2xl font-bold flex items-center gap-2">
              🎬 Timeline Visualizer
            </h2>
            <p className="text-indigo-100 text-sm mt-1">Watch your robot fleet execute tasks in real-time</p>
          </div>
          <button
            onClick={onClose}
            className="bg-white bg-opacity-20 hover:bg-opacity-30 rounded-lg p-2 transition-all"
          >
            <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
            </svg>
          </button>
        </div>

        {/* Video Controls */}
        <div className="bg-slate-50 border-b border-slate-200 p-4 space-y-3">
          {/* Playback Controls */}
          <div className="flex items-center gap-4">
            <button
              onClick={handlePlayPause}
              className="bg-indigo-600 hover:bg-indigo-700 text-white px-6 py-2 rounded-lg font-medium flex items-center gap-2 transition-all"
            >
              {isPlaying ? (
                <>
                  <svg className="w-5 h-5" fill="currentColor" viewBox="0 0 20 20">
                    <path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zM7 8a1 1 0 012 0v4a1 1 0 11-2 0V8zm5-1a1 1 0 00-1 1v4a1 1 0 102 0V8a1 1 0 00-1-1z" clipRule="evenodd" />
                  </svg>
                  Pause
                </>
              ) : (
                <>
                  <svg className="w-5 h-5" fill="currentColor" viewBox="0 0 20 20">
                    <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z" clipRule="evenodd" />
                  </svg>
                  Play
                </>
              )}
            </button>

            <button
              onClick={handleReset}
              className="bg-slate-600 hover:bg-slate-700 text-white px-4 py-2 rounded-lg font-medium flex items-center gap-2 transition-all"
            >
              <svg className="w-5 h-5" fill="currentColor" viewBox="0 0 20 20">
                <path fillRule="evenodd" d="M4 2a1 1 0 011 1v2.101a7.002 7.002 0 0111.601 2.566 1 1 0 11-1.885.666A5.002 5.002 0 005.999 7H9a1 1 0 010 2H4a1 1 0 01-1-1V3a1 1 0 011-1zm.008 9.057a1 1 0 011.276.61A5.002 5.002 0 0014.001 13H11a1 1 0 110-2h5a1 1 0 011 1v5a1 1 0 11-2 0v-2.101a7.002 7.002 0 01-11.601-2.566 1 1 0 01.61-1.276z" clipRule="evenodd" />
              </svg>
              Reset
            </button>

            {/* Speed Controls */}
            <div className="flex items-center gap-2 ml-4">
              <span className="text-sm font-medium text-slate-700">Speed:</span>
              {[0.5, 1, 2, 5, 10].map(speed => (
                <button
                  key={speed}
                  onClick={() => handleSpeedChange(speed)}
                  className={`px-3 py-1 rounded-lg text-sm font-medium transition-all ${
                    playbackSpeed === speed
                      ? 'bg-indigo-600 text-white'
                      : 'bg-white text-slate-700 hover:bg-slate-100 border border-slate-300'
                  }`}
                >
                  {speed}x
                </button>
              ))}
            </div>

            {/* Time Display */}
            <div className="ml-auto text-sm font-mono bg-white px-4 py-2 rounded-lg border border-slate-300">
              <span className="text-indigo-600 font-bold">{currentTime.toFixed(2)}s</span>
              <span className="text-slate-400"> / </span>
              <span className="text-slate-700">{makespan.toFixed(2)}s</span>
            </div>
          </div>

          {/* Timeline Scrubber */}
          <div className="relative">
            <input
              type="range"
              min="0"
              max={makespan}
              step="0.01"
              value={currentTime}
              onChange={handleSeek}
              className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer accent-indigo-600"
            />
            <div className="flex justify-between mt-1 text-xs text-slate-500">
              <span>0s</span>
              <span>{(makespan / 2).toFixed(1)}s</span>
              <span>{makespan.toFixed(1)}s</span>
            </div>
          </div>
        </div>

        {/* Main Content */}
        <div className="flex-1 overflow-y-auto p-6">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            {/* Animated Timeline */}
            <div className="space-y-4">
              <div className="flex items-center justify-between">
                <h3 className="text-lg font-bold text-slate-900 flex items-center gap-2">
                  <svg className="w-5 h-5 text-indigo-600" fill="currentColor" viewBox="0 0 20 20">
                    <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm1-12a1 1 0 10-2 0v4a1 1 0 00.293.707l2.828 2.829a1 1 0 101.415-1.415L11 9.586V6z" clipRule="evenodd" />
                  </svg>
                  Real-Time Timeline
                </h3>
                
                {/* Auto-scroll toggle button */}
                {!autoScroll && (
                  <button
                    onClick={() => setAutoScroll(true)}
                    className="flex items-center gap-2 px-3 py-1.5 bg-indigo-100 hover:bg-indigo-200 text-indigo-700 rounded-lg text-sm font-medium transition-colors"
                  >
                    <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
                    </svg>
                    Enable Auto-Scroll
                  </button>
                )}
              </div>
              
              <div className="space-y-3">
                {robots.map(robot => (
                  <div key={robot.id} className="bg-white rounded-lg border border-slate-200 p-3">
                    <div className="flex items-center gap-3 mb-2">
                      <div className="text-sm font-bold text-slate-700 bg-slate-100 px-2 py-1 rounded">
                        Robot {robot.id}
                      </div>
                      <div className="text-xs text-slate-500">
                        {getCompletedEvents(robot.id).length} / {robot.tasks.length} completed
                      </div>
                    </div>
                    
                    <div className="flex items-center gap-2">
                      {/* Left Arrow */}
                      <button
                        id={`left-arrow-${robot.id}`}
                        className="flex-shrink-0 bg-slate-200 hover:bg-slate-300 active:bg-slate-400 text-slate-700 rounded p-1 transition-colors select-none"
                        style={{ touchAction: 'none' }}
                        title="Scroll left (hold to scroll continuously)"
                      >
                        <svg className="w-5 h-5 pointer-events-none" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
                        </svg>
                      </button>

                      {/* Timeline */}
                      <div className="flex-1 relative h-10 bg-slate-100 rounded overflow-hidden" style={{ maxWidth: '100%' }}>
                        <div 
                          className="relative h-full overflow-x-scroll scrollbar-hide"
                          id={`timeline-scroll-${robot.id}`}
                        >
                          <div className="relative h-full" style={{ width: `${Math.max(100, makespan * 10)}px` }}>
                            {robot.tasks.map((event, idx) => {
                              const widthPx = Math.max(20, event.duration * 10);
                              const leftPx = event.startTime * 10;
                              const status = getEventStatus(event);
                              const progress = getEventProgress(event);

                              if (event.type === 'charging') {
                                // Calculate charging-only block (excluding travel time)
                                const chargingOnlyWidth = Math.max(20, event.chargingTime * 10);
                                const chargingStartPx = (event.startTime + event.travelTime) * 10;
                                
                                return (
                                  <div
                                    key={`charging-${idx}`}
                                    className="absolute h-8 top-1 rounded overflow-hidden border-2 border-yellow-600"
                                    style={{
                                      left: `${chargingStartPx}px`,
                                      width: `${chargingOnlyWidth}px`,
                                    }}
                                  >
                                    <div
                                      className="h-full bg-yellow-500 flex items-center justify-center text-xs text-white font-bold"
                                      style={{
                                        width: status === 'active' ? `${progress}%` : status === 'completed' ? '100%' : '0%',
                                      }}
                                    >
                                      {status === 'active' && '⚡'}
                                    </div>
                                  </div>
                                );
                              }

                              return (
                                <div
                                  key={event.id}
                                  className="absolute h-8 top-1 rounded overflow-hidden"
                                  style={{
                                    left: `${leftPx}px`,
                                    width: `${widthPx}px`,
                                  }}
                                >
                                  <div
                                    className={`h-full flex items-center justify-center text-xs text-white font-bold ${
                                      idx % 4 === 0 ? 'bg-blue-500' : 
                                      idx % 4 === 1 ? 'bg-green-500' : 
                                      idx % 4 === 2 ? 'bg-purple-500' : 'bg-orange-500'
                                    }`}
                                    style={{
                                      width: status === 'active' ? `${progress}%` : status === 'completed' ? '100%' : '0%',
                                    }}
                                  >
                                    {status === 'completed' && event.id.replace('T', '')}
                                  </div>
                                </div>
                              );
                            })}
                          </div>
                        </div>
                      </div>

                      {/* Right Arrow */}
                      <button
                        id={`right-arrow-${robot.id}`}
                        className="flex-shrink-0 bg-slate-200 hover:bg-slate-300 active:bg-slate-400 text-slate-700 rounded p-1 transition-colors select-none"
                        style={{ touchAction: 'none' }}
                        title="Scroll right (hold to scroll continuously, disables auto-scroll)"
                      >
                        <svg className="w-5 h-5 pointer-events-none" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
                        </svg>
                      </button>
                    </div>
                  </div>
                ))}
              </div>
            </div>

            {/* Completed Events List */}
            <div className="space-y-4">
              <h3 className="text-lg font-bold text-slate-900 flex items-center gap-2">
                <svg className="w-5 h-5 text-green-600" fill="currentColor" viewBox="0 0 20 20">
                  <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clipRule="evenodd" />
                </svg>
                Completed Events
              </h3>

              <div className="space-y-4 max-h-[600px] overflow-y-auto">
                {robots.map(robot => {
                  const completedEvents = getCompletedEvents(robot.id);
                  if (completedEvents.length === 0) return null;

                  return (
                    <div key={robot.id} className="bg-white rounded-lg border-t-4 border-blue-500 shadow-md">
                      <div className="bg-gradient-to-r from-blue-50 to-blue-100 p-3 border-b border-blue-200">
                        <h4 className="text-lg font-bold text-blue-900">Robot {robot.id}</h4>
                        <div className="text-sm text-blue-700">
                          {completedEvents.length} event{completedEvents.length !== 1 ? 's' : ''} completed
                        </div>
                      </div>
                      
                      <div className="p-3 space-y-2">
                        {completedEvents.map((event, idx) => {
                          if (event.type === 'charging') {
                            return (
                              <div key={`charging-${idx}`} className="border border-yellow-400 bg-yellow-50 rounded p-2 text-xs animate-fadeIn">
                                <div className="font-bold text-yellow-900 flex items-center gap-1">
                                  ⚡ CHARGING
                                </div>
                                <div className="text-yellow-700 mt-1">
                                  Battery: {event.batteryBefore.toFixed(1)}% → {event.batteryAfter.toFixed(1)}%
                                </div>
                                <div className="text-yellow-600 text-[10px]">
                                  Duration: {event.duration.toFixed(1)}s
                                </div>
                              </div>
                            );
                          }

                          const task = event as Task;
                          return (
                            <div key={task.id} className="border border-slate-200 bg-slate-50 rounded p-2 text-xs animate-fadeIn">
                              <div className="font-bold text-slate-900">{task.id}</div>
                              <div className="text-slate-600">
                                Node {task.fromNode} → Node {task.toNode}
                              </div>
                              <div className="text-slate-500 text-[10px]">
                                Battery: {task.batteryLevel.toFixed(1)}% • Time: {task.cumulativeTime.toFixed(1)}s
                              </div>
                            </div>
                          );
                        })}
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
