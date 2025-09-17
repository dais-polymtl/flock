import { useEffect, useState } from 'react';
import { navLinks } from "@site/src/constants";
import { Reveal } from '../Reveal';
import { CiMenuFries } from 'react-icons/ci';

import React from 'react';
import ThemeToggler from './ThemeToggler';
import ThemeLogo from '../global/ThemeLogo';

const Navbar: React.FC = () => {
    const [toggle, setToggle] = useState(false);
    const [scrolling, setScrolling] = useState(false);

    useEffect(() => {
        const handleScroll = () => {
            if (window.scrollY > 50) {
                setScrolling(true);
            } else {
                setScrolling(false);
            }
        };

        window.addEventListener("scroll", handleScroll);
        return () => window.removeEventListener("scroll", handleScroll);
    }, []);

    return (
        <>
            <nav
                className={`fixed lg:max-w-[1280px] w-full py-4 px-8 mt-4 justify-between z-50 flex items-center rounded-full ${scrolling ? "shadow-xl backdrop-filter backdrop-blur-md" : "bg-transparent"
                }`}
            >
                <Reveal>
                    <ThemeLogo />
                </Reveal>
                <Reveal>
                    <ul className="list-none sm:flex hidden justify-end items-center flex-1">
                        {navLinks.map((nav) => (
                            <li
                                key={nav.id}
                                className="font-poppins font-normal cursor-pointer text-[16px] mr-10">
                                <a href={`#${nav.id}`} className='text-black dark:text-white'>
                                    {nav.title}
                                </a>
                            </li>
                        ))}
                        <li className="mr-10">
                            <ThemeToggler />
                        </li>
                    </ul>
                </Reveal>
                <div className="sm:hidden flex flex-1 justify-end items-center">
                    <Reveal>
                        <div className="mr-4">
                            <ThemeToggler />
                        </div>
                    </Reveal>
                    <Reveal>
                        <CiMenuFries size={28} onClick={() => setToggle((prev) => !prev)} className='flex' />
                    </Reveal>
                    <div className={`${toggle ? 'flex' : 'hidden'}
            p-6 absolute top-20 ring-0 mx-4 my-2 min-w-[140px] rounded-xl sidebar shadow-xl bg-white dark:bg-black`}>
                        <ul className="list-none flex flex-col p-0 justify-end items-center flex-1">
                            {navLinks.map((nav, index) => (
                                <li
                                    key={nav.id}
                                    className={`font-poppins font-normal cursor-pointer text-[16px] ${index === navLinks.length - 1 ? 'mr-0' : 'mb-4'}`}>
                                    <a href={`#${nav.id}`} className='text-black dark:text-white'>
                                        {nav.title}
                                    </a>
                                </li>
                            ))}
                        </ul>
                    </div>
                </div>
            </nav>
            <div className="w-full h-32"></div>
        </>
    )
}

export default Navbar