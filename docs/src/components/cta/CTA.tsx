import styles from "@site/src/css/style";
import {Reveal} from "../Reveal";
import Button from "./buttons";

const CTA: React.FC = () => (
    <section
        className={`${styles.flexCenter} ${styles.marginY} ${styles.padding} sm:flex-row text-white flex-col bg-[#FF9128] rounded-[20px] box-shadow`}>
        <div className="flex-1 flex flex-col">
            <Reveal>
                <h2 className={styles.heading2}>
                    Get Started with Flock Today!
                </h2>
            </Reveal>
            <Reveal>
                <p className={`${styles.paragraph} max-w-[600px] mt-5`}>
                    Unlock the full potential of your data workflows with Flock—your all-in-one solution for advanced
                    AI-driven insights and seamless SQL integration.
                </p>
            </Reveal>
        </div>
        <div className="sm:ml-10 ml-0 sm:mt-0 mt-10">
            <Button href="./docs/what-is-flock"
                    className={`${styles.animate} border-none bg-white text-[#FF9128] hover:bg-[#ffb876] hover:text-white`}>Get
                Started</Button>
        </div>
    </section>
)

export default CTA;