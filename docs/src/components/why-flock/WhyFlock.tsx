import styles, {layout} from "@site/src/css/style";
import AnimatedCodeBlockToTable from "./AnimatedCode2Table";
import {Reveal} from "../Reveal";
import BulletPoint from "./BulletPoint";
import {whyFlock} from "@site/src/constants";
import {useEffect, useState} from "react";

const BulletPoints = ({content}) => {
    const [animatedFeature, setAnimatedFeature] = useState(0);

    useEffect(() => {
        const intervalId = setInterval(() => {
            setAnimatedFeature((prevIdx) => {
                const newIdx = prevIdx + 1;
                return newIdx === content.length ? 0 : newIdx;
            });
        }, 1500); // Delay for each index

        return () => clearInterval(intervalId); // Clean up interval on unmount
    }, [content.length]);

    return (
        <ul className={`${styles.paragraph} flex flex-col items-center md:items-start list-none p-0 max-w-[470px] mt-5 ${styles.animate}`}>
            {content.map((point, index) => (
                <li className={`${index === animatedFeature
                    ? 'bg-[#ffe7d1] dark:bg-[#613205] rounded-2xl' // Show and animate with background
                    : 'bg-transparent'
                } ${styles.animate}`}>
                    <BulletPoint>{point.content}</BulletPoint>
                </li>
            ))}
        </ul>
    )
}

const WhyFlock: React.FC = () => {

    return (
        <section id="why-flock" className={layout.sectionReverse}>
            <div className={layout.sectionImgReverse}>
                <AnimatedCodeBlockToTable/>
            </div>
            <div className={`${layout.sectionInfo}`}>
                <Reveal><h2 className={styles.heading2}>
                    Why Use Flock?
                </h2></Reveal>
                <BulletPoints content={whyFlock}/>
            </div>
        </section>
    )
};

export default WhyFlock;
