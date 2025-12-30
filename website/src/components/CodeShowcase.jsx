import { motion } from 'framer-motion'
import { useInView } from 'framer-motion'
import { useRef, useState } from 'react'
import { FaCopy, FaCheck } from 'react-icons/fa'
import './CodeShowcase.css'

const codeExamples = [
  {
    title: 'Command Substitution',
    code: `user:~$ echo "Today is $(date +%A)"
Today is Sunday

user:~$ files=$(ls | wc -l)
user:~$ echo "Found $files files"
Found 42 files`,
    language: 'shell'
  },
  {
    title: 'Pipelines & Redirection',
    code: `user:~$ cat file.txt | grep "error" | wc -l > count.txt

user:~$ ls -la | sort -r | head -n 10

user:~$ make 2> errors.log`,
    language: 'shell'
  },
  {
    title: 'Job Control',
    code: `user:~$ sleep 100 &
[1] 12345

user:~$ jobs
[1]  Running                 sleep 100 &

user:~$ fg 1
sleep 100
^Z
[1]+ Stopped   sleep 100

user:~$ bg 1
[1]+ sleep 100 &`,
    language: 'shell'
  },
  {
    title: 'Heredoc',
    code: `user:~$ cat <<EOF
> Line 1
> Line 2
> EOF
Line 1
Line 2

user:~$ python3 <<CODE
> print("Hello from heredoc!")
> CODE
Hello from heredoc!`,
    language: 'shell'
  }
]

const CodeBlock = ({ example, index }) => {
  const ref = useRef(null)
  const isInView = useInView(ref, { once: true, margin: "-50px" })
  const [copied, setCopied] = useState(false)

  const handleCopy = () => {
    navigator.clipboard.writeText(example.code)
    setCopied(true)
    setTimeout(() => setCopied(false), 2000)
  }

  return (
    <motion.div
      ref={ref}
      initial={{ opacity: 0, x: index % 2 === 0 ? -50 : 50 }}
      animate={isInView ? { opacity: 1, x: 0 } : { opacity: 0, x: index % 2 === 0 ? -50 : 50 }}
      transition={{ duration: 0.6, delay: index * 0.1 }}
      className="code-example"
    >
      <div className="code-header">
        <h3>{example.title}</h3>
        <motion.button
          whileHover={{ scale: 1.1 }}
          whileTap={{ scale: 0.9 }}
          onClick={handleCopy}
          className="copy-button"
        >
          {copied ? <FaCheck /> : <FaCopy />}
        </motion.button>
      </div>
      <div className="terminal">
        <div className="terminal-dots">
          <div className="terminal-dot"></div>
          <div className="terminal-dot"></div>
          <div className="terminal-dot"></div>
        </div>
        <div className="terminal-content">
          <pre><code>{example.code}</code></pre>
        </div>
      </div>
    </motion.div>
  )
}

const CodeShowcase = () => {
  const ref = useRef(null)
  const isInView = useInView(ref, { once: true, margin: "-100px" })

  return (
    <section className="code-showcase">
      <div className="container">
        <motion.div
          initial={{ opacity: 0, y: 30 }}
          animate={isInView ? { opacity: 1, y: 0 } : { opacity: 0, y: 30 }}
          transition={{ duration: 0.8 }}
        >
          <h2 className="section-title">See It In Action</h2>
          <p className="section-subtitle">
            Real examples of advanced shell features
          </p>
        </motion.div>

        <div className="code-examples-grid">
          {codeExamples.map((example, index) => (
            <CodeBlock key={index} example={example} index={index} />
          ))}
        </div>
      </div>
    </section>
  )
}

export default CodeShowcase
