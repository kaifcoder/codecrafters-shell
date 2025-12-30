import { motion } from 'framer-motion'
import { FaGithub, FaTerminal, FaRocket } from 'react-icons/fa'
import './Hero.css'

const Hero = ({ scrollY }) => {
  return (
    <section className="hero">
      {/* Animated background particles */}
      <div className="particles">
        {[...Array(20)].map((_, i) => (
          <motion.div
            key={i}
            className="particle"
            style={{
              width: Math.random() * 4 + 2,
              height: Math.random() * 4 + 2,
              left: `${Math.random() * 100}%`,
              animationDelay: `${Math.random() * 20}s`,
              animationDuration: `${Math.random() * 10 + 15}s`
            }}
          />
        ))}
      </div>

      <div className="hero-container container">
        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8 }}
          className="hero-content"
        >
          <motion.div
            initial={{ scale: 0 }}
            animate={{ scale: 1 }}
            transition={{ delay: 0.2, type: "spring", stiffness: 200 }}
            className="hero-icon"
          >
            <FaTerminal size={60} />
          </motion.div>

          <motion.h1
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.3, duration: 0.8 }}
            className="hero-title"
          >
            The Next-Gen
            <span className="gradient-text"> C++ Shell</span>
          </motion.h1>

          <motion.p
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.5, duration: 0.8 }}
            className="hero-subtitle"
          >
            A powerful, modular POSIX-compliant shell with advanced features.
            Built with modern C++17 for performance and elegance.
          </motion.p>

          <motion.div
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.7, duration: 0.8 }}
            className="hero-buttons"
          >
            <a href="#installation" className="btn btn-primary">
              <FaRocket /> Get Started
            </a>
            <a 
              href="https://github.com/kaifcoder/codecrafters-shell" 
              className="btn btn-secondary"
              target="_blank"
              rel="noopener noreferrer"
            >
              <FaGithub /> View on GitHub
            </a>
          </motion.div>

          <motion.div
            initial={{ opacity: 0, scale: 0.8 }}
            animate={{ opacity: 1, scale: 1 }}
            transition={{ delay: 0.9, duration: 0.8 }}
            className="hero-stats"
          >
            <div className="stat">
              <div className="stat-number gradient-text">1,500+</div>
              <div className="stat-label">Lines of Code</div>
            </div>
            <div className="stat">
              <div className="stat-number gradient-text">9</div>
              <div className="stat-label">Modules</div>
            </div>
            <div className="stat">
              <div className="stat-number gradient-text">10</div>
              <div className="stat-label">Builtins</div>
            </div>
          </motion.div>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, x: 50 }}
          animate={{ opacity: 1, x: 0 }}
          transition={{ delay: 1, duration: 0.8 }}
          className="hero-terminal"
          style={{ transform: `translateY(${scrollY * 0.1}px)` }}
        >
          <div className="terminal">
            <div className="terminal-dots">
              <div className="terminal-dot"></div>
              <div className="terminal-dot"></div>
              <div className="terminal-dot"></div>
            </div>
            <div className="terminal-content">
              <motion.div
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: 1.2, duration: 0.5 }}
                className="terminal-line"
              >
                <span className="terminal-prompt">$</span>
                <span className="terminal-command">./shell</span>
              </motion.div>
              <motion.div
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: 1.5, duration: 0.5 }}
                className="terminal-output"
              >
                <div>Welcome to Custom C++ Shell</div>
                <div>Features: Job Control • Pipelines • Signals</div>
              </motion.div>
              <motion.div
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: 1.8, duration: 0.5 }}
                className="terminal-line"
              >
                <span className="terminal-prompt">user:~$</span>
                <span className="terminal-command">echo "Hello, World!"</span>
              </motion.div>
              <motion.div
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: 2.1, duration: 0.5 }}
                className="terminal-output"
              >
                Hello, World!
              </motion.div>
              <motion.div
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: 2.4, duration: 0.5 }}
                className="terminal-line"
              >
                <span className="terminal-prompt">user:~$</span>
                <motion.span
                  animate={{ opacity: [1, 0, 1] }}
                  transition={{ repeat: Infinity, duration: 1 }}
                  className="terminal-cursor"
                >
                  ▊
                </motion.span>
              </motion.div>
            </div>
          </div>
        </motion.div>
      </div>

      {/* Scroll indicator */}
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        transition={{ delay: 2.5, duration: 1 }}
        className="scroll-indicator"
      >
        <motion.div
          animate={{ y: [0, 10, 0] }}
          transition={{ repeat: Infinity, duration: 1.5 }}
          className="scroll-arrow"
        >
          ↓
        </motion.div>
      </motion.div>
    </section>
  )
}

export default Hero
