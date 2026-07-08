import styles, {layout} from "@site/src/css/style";
import AnimatedCodeBlockToTable from "./AnimatedCode2Table";
import BulletPoint from "./BulletPoint";
import {whyFlock} from "@site/src/constants";

const WhyFlock: React.FC = () => {
    return (
        <section id="why-flock" className={layout.sectionReverse}>
            <div className={layout.sectionImgReverse}>
                <AnimatedCodeBlockToTable/>
            </div>
            <div className={`${layout.sectionInfo}`}>
                <h2 className={styles.heading2}>
                    Why Use Flock?
                </h2>
                <ul className={`${styles.paragraph} flex flex-col items-center md:items-start list-none p-0 max-w-[470px] mt-5`}>
                    {whyFlock.map((point) => (
                        <li key={point.id}>
                            <BulletPoint>{point.content}</BulletPoint>
                        </li>
                    ))}
                </ul>
            </div>
        </section>
    )
};

export default WhyFlock;
