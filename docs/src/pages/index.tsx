import BrowserOnly from '@docusaurus/BrowserOnly';
import styles from "@site/src/css/style";
import {Navbar, Hero, Features, WhyFlock, Paper, Footer} from "@site/src/components";

const Home: React.FC = () => {
    return (
        <BrowserOnly>
            {() => (
                <body>
                <div className="bg-primary w-full overflow-hidden">
                    <Navbar/>
                    <div className={`bg-primary ${styles.flexStart}`}>
                        <div className={`${styles.boxWidth}`}>
                            <Hero/>
                        </div>
                    </div>
                    <div className={`bg-primary ${styles.paddingX} ${styles.flexStart}`}>
                        <div className={`${styles.boxWidth}`}>
                            <Features/>
                            <WhyFlock/>
                            <Paper/>
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
