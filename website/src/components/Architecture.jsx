import { motion } from 'framer-motion'
import { useInView } from 'framer-motion'
import { useRef } from 'react'
import './Architecture.css'

const modules = [
  { name: 'main.cpp', desc: 'Entry point', size: '127B', color: '#10b981' },
  { name: 'shell.cpp', desc: 'Init & main loop', size: '6.4K', color: '#00d4ff' },
  { name: 'parser.cpp', desc: 'AST & lexer', size: '9.5K', color: '#7c3aed' },
  { name: 'executor.cpp', desc: 'Fork/exec', size: '10K', color: '#f59e0b' },
  { name: 'job_control.cpp', desc: 'Job management', size: '782B', color: '#ec4899' },
  { name: 'builtins.cpp', desc: '10 commands', size: '10K', color: '#6366f1' },
  { name: 'completion.cpp', desc: 'Tab complete', size: '1.2K', color: '#14b8a6' },
  { name: 'heredoc.cpp', desc: 'Heredoc input', size: '557B', color: '#8b5cf6' },
  { name: 'utils.cpp', desc: 'Utilities', size: '2.1K', color: '#f97316' },
]

const ModuleCard = ({ module, index }) => {
  const ref = useRef(null)
  const isInView = useInView(ref, { once: true, margin: "-50px" })

  return (
    <motion.div
      ref={ref}
      initial={{ opacity: 0, scale: 0.8, rotateY: -90 }}
      animate={isInView ? { opacity: 1, scale: 1, rotateY: 0 } : { opacity: 0, scale: 0.8, rotateY: -90 }}
      transition={{ duration: 0.5, delay: index * 0.05 }}
      className="module-card"
      whileHover={{ scale: 1.05, y: -5 }}
    >
      <motion.div
        className="module-dot"
        style={{ background: module.color }}
        animate={{ scale: [1, 1.2, 1] }}
        transition={{ duration: 2, repeat: Infinity, delay: index * 0.1 }}
      />
      <div className="module-name">{module.name}</div>
      <div className="module-desc">{module.desc}</div>
      <div className="module-size">{module.size}</div>
    </motion.div>
  )
}

const Architecture = () => {
  const ref = useRef(null)
  const isInView = useInView(ref, { once: true, margin: "-100px" })

  return (
    <section className="architecture">
      <div className="container">
        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
          transition={{ duration: 0.8 }}
        >
          <h2 className="section-title">Modular Architecture</h2>
          <p className="section-subtitle">
            Clean separation of concerns across 9 independent modules
          </p>
        </motion.div>

        <div className="modules-grid">
          {modules.map((module, index) => (
            <ModuleCard key={index} module={module} index={index} />
          ))}
        </div>

        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
          transition={{ duration: 0.8, delay: 0.5 }}
          className="architecture-benefits"
        >
          <div className="benefit">
            <div className="benefit-number gradient-text">10×</div>
            <div className="benefit-text">Faster Incremental Builds</div>
          </div>
          <div className="benefit">
            <div className="benefit-number gradient-text">1,494</div>
            <div className="benefit-text">Total Lines of Code</div>
          </div>
          <div className="benefit">
            <div className="benefit-number gradient-text">0</div>
            <div className="benefit-text">Circular Dependencies</div>
          </div>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, scale: 0.9 }}
          animate={isInView ? { opacity: 1, scale: 1 } : { opacity: 0, scale: 0.9 }}
          transition={{ duration: 0.8, delay: 0.7 }}
          className="dependency-graph"
        >
          <h3>Dependency Flow</h3>
          <div className="dep-flow">
            <div className="dep-node">main.cpp</div>
            <div className="dep-arrow">→</div>
            <div className="dep-node">shell.cpp</div>
            <div className="dep-arrow">→</div>
            <div className="dep-node">parser.cpp</div>
            <div className="dep-arrow">→</div>
            <div className="dep-node">executor.cpp</div>
          </div>
          <p className="dep-description">
            Clean, unidirectional dependency hierarchy with no circular references
          </p>
        </motion.div>
      </div>
    </section>
  )
}

export default Architecture
