import styles from "@site/src/css/style";
import {Reveal} from "@site/src/components/Reveal";
import {navLinks} from "@site/src/constants";
import Social from "./Social";
import {MdEmail} from "react-icons/md";
import ThemeLogo from "../global/ThemeLogo";

const Footer: React.FC = () => (
    <section className={`${styles.flexCenter} ${styles.paddingY} flex-col`}>
        <footer className="pt-12">
            <div className="max-w-screen-xl mx-auto px-6 grid grid-cols-1 md:grid-cols-3 gap-12">
                {/* Company Info */}
                <div className="space-y-4">
                    <Reveal>
                        <ThemeLogo/>
                    </Reveal>
                    <Reveal>
                        <p className="text-gray-400">
                            Explore the future of data analysis with Flock. Seamlessly integrate AI capabilities into
                            your SQL workflows and unlock powerful insights, all within the flexibility and speed of
                            RDBMSs.
                        </p>
                    </Reveal>
                </div>

                {/* Quick Links */}
                <div className="space-y-4">
                    <Reveal><h3 className="text-xl font-semibold">Quick Links</h3></Reveal>
                    <ul className="space-y-2 p-0">
                        {navLinks.map((link) => (
                            <Reveal>
                                <li><a href={`#${link.id}`}
                                       className="text-gray-400 hover:text-[#FF9128]">{link.title}</a></li>
                            </Reveal>
                        ))}
                    </ul>
                </div>

                <div className="space-y-4">
                    <Reveal><h3 className="text-xl font-semibold">Where to Find Us</h3></Reveal>
                    <Reveal><p className="text-gray-400">We're always ready to connect! Whether you have questions, need
                        support, or just want to learn more, here’s how you can reach us:</p></Reveal>
                    <div className="flex gap-2">
                        <Social Icon={MdEmail} href="mailto:amine.mhedhbi@polymtl.ca,anasdorbani@gmail.com"/>
                    </div>
                </div>
            </div>

            <div className="py-4 mt-12 text-center text-sm flex justify-center">
                <Reveal>
                    <div>
                        This project is developed & maintained by the <a href="https://github.com/dais-polymtl"
                                                                         className="text-[#FF9128] cursor-pointer">Data
                        & AI Systems Laboratory</a> at <a href="https://www.polymtl.ca/"
                                                          className="text-[#FF9128] cursor-pointer">Polytechnique
                        Montréal</a>.
                        <p>&copy; {new Date().getFullYear()} Flock. All rights reserved.</p>
                    </div>
                </Reveal>
            </div>
        </footer>
    </section>
);

export default Footer;
