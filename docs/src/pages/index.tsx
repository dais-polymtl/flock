import BrowserOnly from '@docusaurus/BrowserOnly';
import styles from "@site/src/css/style";
import {Navbar, Hero, Features, WhyFlock, GettingStarted, CTA, Footer, Team} from "@site/src/components";

const Home: React.FC = () => {
    return (
        <BrowserOnly>
            {() => (
                <body>
                <div className="bg-primary w-full overflow-hidden">
                    <div className={`${styles.paddingX} ${styles.flexCenter}`}>
                        <div className={`${styles.boxWidth} flex justify-center`}>
                            <Navbar/>
                        </div>
                    </div>
                    <div className={`bg-primary ${styles.flexStart}`}>
                        <div className={`${styles.boxWidth}`}>
                            <Hero/>
                        </div>
                    </div>
                    <div className={`bg-primary ${styles.paddingX} ${styles.flexStart}`}>
                        <div className={`${styles.boxWidth}`}>
                            <Features/>
                            <WhyFlock/>
                            <GettingStarted/>
                            <Team/>
                            <CTA/>
                            <Footer/>
                        </div>
                    </div>
                </div>
                </body>
            )}
        </BrowserOnly>
    )
}

export default Home