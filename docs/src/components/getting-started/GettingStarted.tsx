import styles, {layout} from "@site/src/css/style";
import {Reveal} from "../Reveal";
import CodeBox from "./CodeBox";

const GettingStarted: React.FC = () => {

    return (
        <section id="getting-started" className={layout.section}>
            <div className={layout.sectionInfo}>
                <Reveal>
                    <h2 className={styles.heading2}>
                        Getting Started
                    </h2>
                </Reveal>
                <Reveal>
                    <p className={`${styles.paragraph} max-w-[470px] mt-5`}>
                        You're just two steps away from getting started. First, install the extension, and then simply
                        load it to begin using all its features. Once you've completed these steps, you'll be ready to
                        dive in and start working with the powerful capabilities of Flock!
                    </p>
                </Reveal>
            </div>
            <div className={`${layout.sectionImg}`}>
                <div className="absolute inset-0 flex justify-center items-center -z-0">
                    <div className="w-96 h-96 border-2 border-orange-500 opacity-30 rounded-full"></div>
                    {/* Middle Circle */}
                    <div className="absolute w-72 h-72 border-4 border-orange-500 opacity-40 rounded-full"></div>
                    {/* Inner Circle */}
                    <div className="absolute w-48 h-48 border-8 border-orange-500 opacity-50 rounded-full"></div>
                </div>
                <CodeBox/>
            </div>
        </section>
    )
};

export default GettingStarted;
