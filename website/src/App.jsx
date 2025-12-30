import { useState, useEffect } from 'react'
import Hero from './components/Hero'
import Features from './components/Features'
import Architecture from './components/Architecture'
import CodeShowcase from './components/CodeShowcase'
import Installation from './components/Installation'
import Footer from './components/Footer'
import './App.css'

function App() {
  const [scrollY, setScrollY] = useState(0)

  useEffect(() => {
    const handleScroll = () => setScrollY(window.scrollY)
    window.addEventListener('scroll', handleScroll)
    return () => window.removeEventListener('scroll', handleScroll)
  }, [])

  return (
    <div className="app">
      <Hero scrollY={scrollY} />
      <Features />
      <Architecture />
      <CodeShowcase />
      <Installation />
      <Footer />
    </div>
  )
}

export default App
