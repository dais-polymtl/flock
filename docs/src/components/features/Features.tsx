import styles, {layout} from "@site/src/css/style";
import {features} from "@site/src/constants";
import {FeatredCardProps} from "@site/src/types";

const OverviewCard: React.FC<FeatredCardProps> = ({Icon, title, content}) => (
    <div className="flex flex-row p-6 gap-4">
        <div
            className={`w-[64px] h-[64px] rounded-full bg-[#ffe7d1] text-[#FF9128] ${styles.flexCenter} shrink-0`}
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

const Features: React.FC = () => (
    <section id="overview" className={`${layout.section}`}>
        <div className={layout.sectionInfo}>
            <h2 className={styles.heading2}>
                Overview
            </h2>
            <p className={`${styles.paragraph} max-w-[470px] mt-5`}>
                Flock brings language models and retrieval into DuckDB so semantic analysis
                runs alongside ordinary SQL analytics.
            </p>
        </div>
        <div className={`${layout.sectionImg} flex-col`}>
            {features.map((feature, index) => (
                <OverviewCard key={feature.id} Icon={feature.icon} {...feature} index={index}/>
            ))}
        </div>
    </section>
);

export default Features;
