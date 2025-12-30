import { motion } from 'framer-motion'
import { useInView } from 'framer-motion'
import { useRef } from 'react'
import { 
  FaBolt, 
  FaLayerGroup, 
  FaCogs, 
  FaTerminal, 
  FaExchangeAlt, 
  FaSignal 
} from 'react-icons/fa'
import './Features.css'

const features = [
  {
    icon: <FaBolt />,
    title: 'Lightning Fast',
    description: 'Modular compilation with 10× faster incremental builds. Only recompile what changed.',
    color: '#f59e0b'
  },
  {
    icon: <FaLayerGroup />,
    title: 'Modular Architecture',
    description: '9 independent modules for clean separation of concerns. Easy to maintain and extend.',
    color: '#00d4ff'
  },
  {
    icon: <FaCogs />,
    title: 'Job Control',
    description: 'Full background job support with fg, bg, and jobs commands. POSIX-compliant process management.',
    color: '#7c3aed'
  },
  {
    icon: <FaTerminal />,
    title: 'Advanced Parsing',
    description: 'AST-based parser with command substitution $(…), pipelines, and heredocs.',
    color: '#10b981'
  },
  {
    icon: <FaExchangeAlt />,
    title: 'I/O Redirection',
    description: 'Complete redirection support: >, >>, 2>, <, and heredocs (<<). Full pipeline support.',
    color: '#ec4899'
  },
  {
    icon: <FaSignal />,
    title: 'Signal Handling',
    description: 'Proper SIGINT, SIGTSTP, and SIGCHLD handling. Terminal control with job suspension.',
    color: '#6366f1'
  }
]

const FeatureCard = ({ feature, index }) => {
  const ref = useRef(null)
  const isInView = useInView(ref, { once: true, margin: "-100px" })

  return (
    <motion.div
      ref={ref}
      initial={{ opacity: 0, y: 50 }}
      animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 50 }}
      transition={{ duration: 0.6, delay: index * 0.1 }}
      className="feature-card"
      whileHover={{ y: -10, transition: { duration: 0.2 } }}
    >
      <motion.div
        className="feature-icon"
        style={{ color: feature.color }}
        whileHover={{ scale: 1.1, rotate: 5 }}
        transition={{ type: "spring", stiffness: 300 }}
      >
        {feature.icon}
      </motion.div>
      <h3 className="feature-title">{feature.title}</h3>
      <p className="feature-description">{feature.description}</p>
      <motion.div
        className="feature-glow"
        style={{ background: `radial-gradient(circle, ${feature.color}33 0%, transparent 70%)` }}
        animate={{ opacity: [0.3, 0.6, 0.3] }}
        transition={{ duration: 3, repeat: Infinity }}
      />
    </motion.div>
  )
}

const Features = () => {
  const ref = useRef(null)
  const isInView = useInView(ref, { once: true, margin: "-100px" })

  return (
    <section className="features" ref={ref}>
      <div className="container">
        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
          transition={{ duration: 0.8 }}
        >
          <h2 className="section-title">Powerful Features</h2>
          <p className="section-subtitle">
            Everything you need in a modern shell, built with C++17
          </p>
        </motion.div>

        <div className="features-grid">
          {features.map((feature, index) => (
            <FeatureCard key={index} feature={feature} index={index} />
          ))}
        </div>

        {/* Additional features list */}
        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
          transition={{ duration: 0.8, delay: 0.6 }}
          className="additional-features"
        >
          <h3>Plus More...</h3>
          <div className="features-list">
            <div className="feature-item">✓ Tab completion for commands</div>
            <div className="feature-item">✓ Command history with readline</div>
            <div className="feature-item">✓ 10 builtin commands</div>
            <div className="feature-item">✓ Colored prompts</div>
            <div className="feature-item">✓ POSIX-compliant</div>
            <div className="feature-item">✓ Cross-platform (Linux/macOS)</div>
          </div>
        </motion.div>
      </div>
    </section>
  )
}

export default Features
