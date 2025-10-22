import React from 'react';
import './RobotIcon.css';

interface RobotIconProps {
  darkMode: boolean;
}

const RobotIcon: React.FC<RobotIconProps> = ({ darkMode }) => {
  return (
    <div className="robot-container">
      <svg
        width="32" // Smaller size for timeline
        height="38" // Smaller height
        viewBox="0 0 100 120" // Keep viewBox for scaling
        className="robot-svg"
      >
        {/* Main Body - Mini P.E.K.K.A. armor */}
        <g className="mini-pekka-body">
          {/* Main chest piece */}
          <path d="M 20 45 C 10 50, 10 80, 20 85 L 80 85 C 90 80, 90 50, 80 45 Z" className="body-main" />
          {/* Shoulder pads */}
          <circle cx="20" cy="55" r="10" className="shoulder-pad-left" />
          <circle cx="80" cy="55" r="10" className="shoulder-pad-right" />
          {/* Belt/Waist */}
          <rect x="25" y="80" width="50" height="8" rx="4" className="body-belt" />
        </g>

        {/* Legs - Mini P.E.K.K.A. style with walking animation */}
        <g className="mini-pekka-legs">
          <g className="leg-left-group">
            <rect x="30" y="88" width="12" height="20" rx="6" className="leg-left" />
            {/* Feet */}
            <rect x="28" y="105" width="16" height="8" rx="4" className="foot-left" />
          </g>
          <g className="leg-right-group">
            <rect x="58" y="88" width="12" height="20" rx="6" className="leg-right" />
            <rect x="56" y="105" width="16" height="8" rx="4" className="foot-right" />
          </g>
        </g>

        {/* Head - Mini P.E.K.K.A. helmet */}
        <g className="mini-pekka-head">
          <path d="M 30 20 C 20 30, 20 50, 30 60 L 70 60 C 80 50, 80 30, 70 20 Z" className="head-helmet" />
          {/* Eye Visor */}
          <path d="M 38 35 Q 45 30 50 35 Q 55 30 62 35 C 60 45, 40 45, 38 35 Z" className="head-visor" />
          {/* Horns */}
          <path d="M 35 25 Q 28 10 35 5 L 40 15 Z" className="head-horn-left" />
          <path d="M 65 25 Q 72 10 65 5 L 60 15 Z" className="head-horn-right" />
        </g>

        {/* Arms - Mini P.E.K.K.A. */}
        <g className="mini-pekka-arms">
          {/* Left Arm */}
          <rect x="10" y="50" width="15" height="30" rx="7.5" className="arm-left" />

          {/* Right Arm */}
          <rect x="75" y="50" width="15" height="30" rx="7.5" className="arm-right" />
        </g>
      </svg>

      {/* Shadow effect */}
      <div className="robot-shadow"></div>
    </div>
  );
};

export default RobotIcon;