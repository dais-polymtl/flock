import styles from "@site/src/css/style";
import { FaGithub } from "react-icons/fa";
import { SiGoogledocs } from "react-icons/si";
import { GrArticle } from "react-icons/gr";
import Button from "./buttons";

const Hero: React.FC = () => {
    return (
        <section id="home" className="flex flex-col w-full justify-center items-center text-center">
            <div className="flex flex-col w-full justify-center items-center text-center">
                <p className="text-sm tracking-wide uppercase text-[#FF9128] font-medium mt-8 mb-2">
                    Proc. VLDB Endow. 2025 · Vol. 18, No. 12
                </p>
                <h1 className="md:text-5xl text-3xl font-bold text-black dark:text-white max-w-4xl leading-tight">
                    Beyond Quacking: Deep Integration of Language Models and RAG into DuckDB
                </h1>
                <p className="mt-4 text-base text-gray-600 dark:text-gray-300">
                    Anas Dorbani · Sunny Yasser · Jimmy Lin · Amine Mhedhbi
                </p>
                <p className={`${styles.paragraph} max-w-[600px] mt-6`}>
                    Flock is a DuckDB extension that deeply integrates LLM and RAG capabilities into
                    OLAP systems, so you can mix analytics with semantic analysis in SQL using map and
                    reduce functions for generation, classification, embeddings, and more.
                </p>
            </div>
            <div className="grid md:grid-rows-1 grid-rows-2 grid-flow-col md:gap-8 gap-2 py-8 place-items-center">
                <Button
                    href="https://dl.acm.org/doi/10.14778/3750601.3750685"
                    className="text-[#ffe7d1] bg-[#FF9128] hover:bg-[#ffe7d1] hover:text-[#FF9128]"
                >
                    <GrArticle />Paper
                </Button>
                <Button
                    href="https://github.com/dais-polymtl/flock"
                    className="bg-[#ffe7d1] text-[#FF9128] hover:text-[#ffe7d1] hover:bg-[#FF9128]"
                >
                    <FaGithub />Source Code
                </Button>
                <Button
                    href="./docs/what-is-flock"
                    className="text-[#FF9128] border-2 border-solid border-[#FF9128] hover:bg-[#ffe7d1] hover:text-[#FF9128] hover:border-[#ffe7d1]"
                >
                    <SiGoogledocs />Docs
                </Button>
            </div>
        </section>
    );
};

export default Hero;
