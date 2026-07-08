import styles, {layout} from "@site/src/css/style";
import CodeBox from "./CodeBox";

const GettingStarted: React.FC = () => {
    return (
        <section id="getting-started" className={layout.section}>
            <div className={layout.sectionInfo}>
                <h2 className={styles.heading2}>
                    Getting Started
                </h2>
                <p className={`${styles.paragraph} max-w-[470px] mt-5`}>
                    Install the DuckDB extension, then load it to start using Flock from SQL.
                    See the documentation for models, prompts, and function reference.
                </p>
            </div>
            <div className={`${layout.sectionImg}`}>
                <div className="absolute inset-0 flex justify-center items-center -z-0">
                    <div className="w-96 h-96 border-2 border-orange-500 opacity-30 rounded-full"></div>
                    <div className="absolute w-72 h-72 border-4 border-orange-500 opacity-40 rounded-full"></div>
                    <div className="absolute w-48 h-48 border-8 border-orange-500 opacity-50 rounded-full"></div>
                </div>
                <CodeBox/>
            </div>
        </section>
    )
};

export default GettingStarted;
