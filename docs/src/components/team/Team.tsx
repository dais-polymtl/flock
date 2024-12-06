import { team } from "@site/src/constants";
import { Reveal } from "../Reveal";
import styles from "@site/src/css/style";
import { FaGithub, FaLinkedin } from "react-icons/fa";
import { FaXTwitter } from "react-icons/fa6";

const Team = () => {
    return (
        <section id="team">
            <div className="py-12 px-6 items-center flex justify-center flex-col w-full">
                <Reveal>
                    <h2 className={`${styles.heading2} text-4xl font-semibold text-center mb-8`}>Team</h2>
                </Reveal>

                <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 w-full gap-8">
                    {team.map((teamMember) => (
                        <div key={teamMember.name} className="p-6 rounded-3xl shadow-xl text-center flex justify-center flex-col items-center gap-8 dark:border-solid dark:border-[1px] dark:border-[#FF9128]">
                            <Reveal>
                                <img src={teamMember.img} alt={teamMember.name} className="w-32 h-32 rounded-full mx-auto mb-4 object-cover" />
                            </Reveal>
                            <Reveal>
                                <div className="flex flex-col items-center gap-2">
                                    <div className="text-xl font-medium">{teamMember.name}</div>

                                    <div className="text-gray-500">{teamMember.role}</div>

                                    <div className="text-gray-500">{teamMember.company}</div>
                                </div>
                            </Reveal>
                            <Reveal>
                                <div className="flex gap-2">
                                    {teamMember.github &&
                                        <a href={teamMember.github} target="_blank" rel="noopener noreferrer" className={`${styles.animate} text-[#FF9128] hover:text-[#ffd4ac]`}>
                                            <FaGithub size={24} />
                                        </a>
                                    }
                                    {teamMember.linkedin &&
                                        <a href={teamMember.linkedin} target="_blank" rel="noopener noreferrer" className={`${styles.animate} text-[#FF9128] hover:text-[#ffd4ac]`}>
                                            <FaLinkedin size={24} />
                                        </a>
                                    }
                                    {teamMember.X &&
                                        <a href={teamMember.X} target="_blank" rel="noopener noreferrer" className={`${styles.animate} text-[#FF9128] hover:text-[#ffd4ac]`}>
                                            <FaXTwitter size={24} />
                                        </a>
                                    }
                                </div>
                            </Reveal>
                        </div>
                    ))}
                </div>
            </div>
        </section>
    );
}

export default Team;