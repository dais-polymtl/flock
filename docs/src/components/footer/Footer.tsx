import styles from "@site/src/css/style";
import {navLinks} from "@site/src/constants";
import Social from "./Social";
import {MdEmail} from "react-icons/md";
import ThemeLogo from "../global/ThemeLogo";

const Footer: React.FC = () => (
    <section className={`${styles.flexCenter} ${styles.paddingY} flex-col`}>
        <footer className="pt-12">
            <div className="max-w-screen-xl mx-auto px-6 grid grid-cols-1 md:grid-cols-3 gap-12">
                <div className="space-y-4">
                    <ThemeLogo/>
                    <p className="text-gray-400">
                        A DuckDB extension for integrating language models and RAG into SQL analytics.
                        Companion site for the VLDB 2025 paper.
                    </p>
                </div>

                <div className="space-y-4">
                    <h3 className="text-xl font-semibold">Quick Links</h3>
                    <ul className="space-y-2 p-0">
                        {navLinks.map((link) => (
                            <li key={link.id}>
                                <a href={link.href} className="text-gray-400 hover:text-[#FF9128]">
                                    {link.title}
                                </a>
                            </li>
                        ))}
                    </ul>
                </div>

                <div className="space-y-4">
                    <h3 className="text-xl font-semibold">Contact</h3>
                    <p className="text-gray-400">Questions about the project or paper? Reach the authors by email.</p>
                    <div className="flex gap-2">
                        <Social Icon={MdEmail} href="mailto:amine.mhedhbi@polymtl.ca,anasdorbani@gmail.com"/>
                    </div>
                </div>
            </div>

            <div className="py-4 mt-12 text-center text-sm flex justify-center">
                <div>
                    This project is developed & maintained by the <a href="https://github.com/dais-polymtl"
                                                                     className="text-[#FF9128] cursor-pointer">Data
                    & AI Systems Laboratory</a> at <a href="https://www.polymtl.ca/"
                                                      className="text-[#FF9128] cursor-pointer">Polytechnique
                    Montréal</a>.
                    <p>&copy; {new Date().getFullYear()} Flock. All rights reserved.</p>
                </div>
            </div>
        </footer>
    </section>
);

export default Footer;
