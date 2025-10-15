import React from 'react';
import './RobotIconSitting.css';

interface RobotIconSittingProps {
  darkMode: boolean;
}

const RobotIconSitting: React.FC<RobotIconSittingProps> = ({ darkMode }) => {
  return (
    <div className="robot-sitting-container">
      <svg
        width="28"
        height="16"
        viewBox="0 0 140 80"
        className="robot-sitting-svg"
      >
        <g className="mini-pekka-body-sitting" transform="translate(45, 20) rotate(90)">
          <path d="M 20 45 C 10 50, 10 80, 20 85 L 80 85 C 90 80, 90 50, 80 45 Z" className="body-main-sitting" />
          <circle cx="20" cy="55" r="10" className="shoulder-pad-left-sitting" />
          <circle cx="80" cy="55" r="10" className="shoulder-pad-right-sitting" />
          <rect x="25" y="80" width="50" height="8" rx="4" className="body-belt-sitting" />
        </g>

        <g className="mini-pekka-head-sitting" transform="translate(20, 20) rotate(80)">
          <path d="M 30 20 C 20 30, 20 50, 30 60 L 70 60 C 80 50, 80 30, 70 20 Z" className="head-helmet-sitting" />
          <path d="M 38 35 Q 45 30 50 35 Q 55 30 62 35 C 60 45, 40 45, 38 35 Z" className="head-visor-sitting" />
          <path d="M 35 25 Q 28 10 35 5 L 40 15 Z" className="head-horn-left-sitting" />
          <path d="M 65 25 Q 72 10 65 5 L 60 15 Z" className="head-horn-right-sitting" />
        </g>

        <g className="mini-pekka-arms-sitting">
          <g transform="translate(60, 25) rotate(70)">
            <rect x="10" y="50" width="15" height="30" rx="7.5" className="arm-left-sitting" />
          </g>

          <g transform="translate(80, 45) rotate(110)">
            <rect x="75" y="50" width="15" height="30" rx="7.5" className="arm-right-sitting" />
          </g>
        </g>

        <g className="mini-pekka-legs-sitting">
          <g className="leg-left-group-sitting" transform="translate(100, 30) rotate(10)">
            <rect x="30" y="88" width="12" height="20" rx="6" className="leg-left-sitting" />
            <rect x="28" y="105" width="16" height="8" rx="4" className="foot-left-sitting" />
          </g>
          <g className="leg-right-group-sitting" transform="translate(100, 35) rotate(10)">
            <rect x="58" y="88" width="12" height="20" rx="6" className="leg-right-sitting" />
            <rect x="56" y="105" width="16" height="8" rx="4" className="foot-right-sitting" />
          </g>
        </g>

      </svg>

      <div className="robot-sitting-shadow"></div>
    </div>
  );
};

export default RobotIconSitting;
