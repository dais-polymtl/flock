import styles from "@site/src/css/style";
import { Reveal } from "../Reveal";
import { FaGithub } from "react-icons/fa";
import { SiGoogledocs } from "react-icons/si";
import Button from "./buttons";
import Typewriter from 'typewriter-effect';

const Hero: React.FC = () => {

  return (
    <section id="home" className="flex flex-col w-full justify-center items-center text-center">
      <div
        className="flex flex-col w-full justify-center items-center text-center"
      >
        <Reveal>
          <div className="md:text-6xl text-4xl font-bold text-[#FF9128] my-8 flex flex-col">
            <span className="md:text-8xl text-6xl">FlockMTL</span> <span className="text-black dark:text-white">
              <Typewriter
                options={{
                  strings: ["INTELLIGENT REPORTING", "TABULAR UNDERSTANDING", "LLMs & RAG WITHIN SQL"],
                  deleteSpeed: .2,
                  autoStart: true,
                  loop: true,
                  delay: 50,
                }}/>
            </span>
          </div>
        </Reveal>
        <Reveal><p className={`${styles.paragraph} max-w-[600px] mt-5`}>
          DBMS extension that deeply integrates LLM and RAG capabilities directly into OLAP systems to seamlessly mix analytics with semantic analysis. It enables tasks like text generation, semantic analysis, classification, and embeddings, all within SQL using novel map and reduce functions. <span className="italic font-medium">FlockMTL redefines how to tackle complex workflows using RDBMSs</span>
        </p></Reveal>
      </div>
      <div className="grid md:grid-rows-1 grid-rows-2 grid-flow-col md:gap-8 gap-2 py-8 place-items-center">
        <Button href="https://github.com/dsg-polymtl/flockmtl-duckdb" className="bg-[#ffe7d1] text-[#FF9128] hover:text-[#ffe7d1] hover:bg-[#FF9128]">
          <FaGithub />Source Code
        </Button>
        <Button href="./docs/what-is-flockmtl" className="text-[#ffe7d1] bg-[#FF9128] hover:bg-[#ffe7d1] hover:text-[#FF9128]">
          <SiGoogledocs href="#" />Documentation
        </Button>
      </div>
    </section>
  )
}

export default Hero;
