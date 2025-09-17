import styles, {layout} from "@site/src/css/style";
import {features} from "@site/src/constants";
import {FeatredCardProps} from "@site/src/types";
import {useEffect, useState} from "react";
import {Reveal} from "../Reveal";

const FeaturesCard: React.FC<FeatredCardProps> = ({Icon, title, content, index}) => (
    <div
        className={`flex flex-row p-6 gap-4`}
    >
        <div
            className={`w-[64px] h-[64px] rounded-full bg-[#ffe7d1] text-[#FF9128] ${styles.flexCenter}`}
        >
            <Icon size={26}/>
        </div>
        <div className="flex-1 flex flex-col ml-3">
            <h4 className="font-poppins font-semibold text-[18px] leading-[24px]">
                {title}
            </h4>
            <p className="font-poppins font-normal text-dimBlack text-[16px] leading-[24px] m-auto">
                {content}
            </p>
        </div>
    </div>
);

const FeaturesSection = ({features}) => {
    const [animatedFeature, setAnimatedFeature] = useState(0);

    useEffect(() => {
        const intervalId = setInterval(() => {
            setAnimatedFeature((prevIdx) => {
                const newIdx = prevIdx + 1;
                return newIdx === features.length ? 0 : newIdx;
            });
        }, 2000);

        return () => clearInterval(intervalId);
    }, [features.length]);

    return (
        <div className={`${layout.sectionImg} flex-col`} style={{position: 'relative'}}>
            {features.map((feature, index) => (
                <div
                    key={feature.id}
                    className={`${index === animatedFeature
                        ? 'bg-[#ffe7d1] dark:bg-[#613205] rounded-[30px]'
                        : 'bg-transparent'
                    } ${styles.animate}`}
                >
                    <Reveal>
                        <FeaturesCard Icon={feature.icon} {...feature} index={index}/>
                    </Reveal>
                </div>
            ))}
        </div>
    );
};


const Features: React.FC = () => (
    <section id="features" className={`${layout.section}`}>
        <div className={layout.sectionInfo}>
            <Reveal><h2 className={styles.heading2}>
                Integrating LM and IR<br className="sm:block hidden"/>
                Capabilities in SQL
            </h2></Reveal>
            <Reveal>
                <p className={`${styles.paragraph} max-w-[470px] mt-5`}>
                    Flock integrates advanced language models into RDBMSs, enabling powerful tasks like text generation,
                    classification, filtering, and embeddings directly through SQL queries. Streamline your data
                    workflows with AI-driven insights.
                </p>
            </Reveal>
        </div>
        <FeaturesSection features={features}/>
    </section>
);

export default Features;
