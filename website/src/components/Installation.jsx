import { motion } from 'framer-motion'
import { useInView } from 'framer-motion'
import { useRef } from 'react'
import { FaDownload, FaGithub, FaTerminal } from 'react-icons/fa'
import './Installation.css'

const Installation = () => {
  const ref = useRef(null)
  const isInView = useInView(ref, { once: true, margin: "-100px" })

  const steps = [
    {
      number: '1',
      title: 'Clone Repository',
      command: 'git clone https://github.com/kaifcoder/codecrafters-shell.git\ncd codecrafters-shell'
    },
    {
      number: '2',
      title: 'Build',
      command: 'make clean && make'
    },
    {
      number: '3',
      title: 'Run',
      command: './shell'
    }
  ]

  return (
    <section className="installation" id="installation">
      <div className="container">
        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
          transition={{ duration: 0.8 }}
        >
          <h2 className="section-title">Get Started</h2>
          <p className="section-subtitle">
            Three simple steps to run your own C++ shell
          </p>
        </motion.div>

        <div className="installation-steps">
          {steps.map((step, index) => (
            <motion.div
              key={index}
              ref={ref}
              initial={{ opacity: 0, y: 30 }}
              animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
              transition={{ duration: 0.6, delay: index * 0.2 }}
              className="installation-step"
            >
              <motion.div
                className="step-number"
                whileHover={{ scale: 1.1, rotate: 5 }}
              >
                {step.number}
              </motion.div>
              <div className="step-content">
                <h3 className="step-title">{step.title}</h3>
                <div className="code-block">
                  <pre><code>{step.command}</code></pre>
                </div>
              </div>
            </motion.div>
          ))}
        </div>

        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
          transition={{ duration: 0.8, delay: 0.6 }}
          className="requirements"
        >
          <h3>Requirements</h3>
          <div className="requirements-grid">
            <div className="requirement">
              <div className="req-icon">🔧</div>
              <div className="req-name">g++ or clang++</div>
              <div className="req-desc">C++17 compatible</div>
            </div>
            <div className="requirement">
              <div className="req-icon">📚</div>
              <div className="req-name">GNU Readline</div>
              <div className="req-desc">libreadline-dev</div>
            </div>
            <div className="requirement">
              <div className="req-icon">⚙️</div>
              <div className="req-name">Make</div>
              <div className="req-desc">Build system</div>
            </div>
            <div className="requirement">
              <div className="req-icon">🐧</div>
              <div className="req-name">Linux/macOS</div>
              <div className="req-desc">POSIX-compliant</div>
            </div>
          </div>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, scale: 0.9 }}
          animate={isInView ? { opacity: 1, scale: 1 } : { opacity: 0, scale: 0.9 }}
          transition={{ duration: 0.8, delay: 0.8 }}
          className="cta-section"
        >
          <h3>Ready to dive in?</h3>
          <p>Check out the source code and contribute to the project</p>
          <div className="cta-buttons">
            <a href="https://github.com/kaifcoder/codecrafters-shell" className="btn btn-primary" target="_blank" rel="noopener noreferrer">
              <FaGithub /> View on GitHub
            </a>
            <a href="https://github.com/kaifcoder/codecrafters-shell/archive/refs/heads/master.zip" className="btn btn-secondary" target="_blank" rel="noopener noreferrer">
              <FaDownload /> Download ZIP
            </a>
          </div>
        </motion.div>
      </div>
    </section>
  )
}

export default Installation
