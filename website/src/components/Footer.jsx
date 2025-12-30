import { motion } from 'framer-motion'
import { FaGithub, FaTwitter, FaLinkedin, FaHeart, FaTerminal } from 'react-icons/fa'
import './Footer.css'

const Footer = () => {
  return (
    <footer className="footer">
      <div className="container">
        <div className="footer-content">
          <div className="footer-section">
            <div className="footer-logo">
              <FaTerminal size={32} />
              <span>C++ Shell</span>
            </div>
            <p className="footer-description">
              A modern, modular POSIX-compliant shell built with C++17
            </p>
            <div className="footer-social">
              <motion.a
                whileHover={{ scale: 1.2, rotate: 5 }}
                href="https://github.com/kaifcoder/codecrafters-shell"
                target="_blank"
                rel="noopener noreferrer"
                className="social-icon"
              >
                <FaGithub />
              </motion.a>
              <motion.a
                whileHover={{ scale: 1.2, rotate: 5 }}
                href="https://twitter.com"
                target="_blank"
                rel="noopener noreferrer"
                className="social-icon"
              >
                <FaTwitter />
              </motion.a>
              <motion.a
                whileHover={{ scale: 1.2, rotate: 5 }}
                href="https://linkedin.com/in/mohdkaif00"
                target="_blank"
                rel="noopener noreferrer"
                className="social-icon"
              >
                <FaLinkedin />
              </motion.a>
            </div>
          </div>

          <div className="footer-section">
            <h4>Documentation</h4>
            <ul>
              <li><a href="#features">Features</a></li>
              <li><a href="#architecture">Architecture</a></li>
              <li><a href="#installation">Installation</a></li>
              <li><a href="https://github.com/kaifcoder/codecrafters-shell#readme">API Reference</a></li>
            </ul>
          </div>

          <div className="footer-section">
            <h4>Resources</h4>
            <ul>
              <li><a href="https://github.com/kaifcoder/codecrafters-shell">GitHub Repository</a></li>
              <li><a href="https://github.com/kaifcoder/codecrafters-shell/issues">Issue Tracker</a></li>
              <li><a href="https://github.com/kaifcoder/codecrafters-shell/blob/master/CONTRIBUTING.md">Contributing</a></li>
              <li><a href="https://github.com/kaifcoder/codecrafters-shell/releases">Changelog</a></li>
            </ul>
          </div>

          <div className="footer-section">
            <h4>Project Stats</h4>
            <div className="footer-stats">
              <div className="footer-stat">
                <div className="stat-value">1,494</div>
                <div className="stat-label">Lines of Code</div>
              </div>
              <div className="footer-stat">
                <div className="stat-value">9</div>
                <div className="stat-label">Modules</div>
              </div>
            </div>
          </div>
        </div>

        <div className="footer-bottom">
          <div className="footer-copyright">
            <p>
              Made with <FaHeart className="heart-icon" /> using C++17, React & Framer Motion
            </p>
            <p>&copy; {new Date().getFullYear()} C++ Shell. All rights reserved.</p>
          </div>
          <div className="footer-links">
            <a href="#privacy">Privacy Policy</a>
            <span>•</span>
            <a href="#terms">Terms of Service</a>
            <span>•</span>
            <a href="#license">MIT License</a>
          </div>
        </div>
      </div>
    </footer>
  )
}

export default Footer
