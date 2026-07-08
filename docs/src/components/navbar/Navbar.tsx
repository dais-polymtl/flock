import { useState } from 'react';
import { navLinks } from "@site/src/constants";
import { CiMenuFries } from 'react-icons/ci';

import React from 'react';
import ThemeToggler from './ThemeToggler';
import ThemeLogo from '../global/ThemeLogo';

const Navbar: React.FC = () => {
    const [toggle, setToggle] = useState(false);

    return (
        <>
            <nav
                className="fixed top-0 left-0 right-0 w-full py-3 px-8 justify-between z-50 flex items-center bg-white dark:bg-black border-b border-gray-200 dark:border-gray-800"
            >
                <ThemeLogo />
                <ul className="list-none sm:flex hidden justify-end items-center flex-1">
                    {navLinks.map((nav) => (
                        <li
                            key={nav.id}
                            className="font-poppins font-normal cursor-pointer text-[16px] mr-10">
                            <a href={nav.href} className='text-black dark:text-white'>
                                {nav.title}
                            </a>
                        </li>
                    ))}
                    <li className="mr-10">
                        <ThemeToggler />
                    </li>
                </ul>
                <div className="sm:hidden flex flex-1 justify-end items-center">
                    <div className="mr-4">
                        <ThemeToggler />
                    </div>
                    <CiMenuFries size={28} onClick={() => setToggle((prev) => !prev)} className='flex' />
                    <div className={`${toggle ? 'flex' : 'hidden'}
            p-6 absolute top-14 right-4 mx-0 my-2 min-w-[140px] sidebar bg-white dark:bg-black border border-gray-200 dark:border-gray-800`}>
                        <ul className="list-none flex flex-col p-0 justify-end items-center flex-1">
                            {navLinks.map((nav, index) => (
                                <li
                                    key={nav.id}
                                    className={`font-poppins font-normal cursor-pointer text-[16px] ${index === navLinks.length - 1 ? 'mr-0' : 'mb-4'}`}>
                                    <a href={nav.href} className='text-black dark:text-white'>
                                        {nav.title}
                                    </a>
                                </li>
                            ))}
                        </ul>
                    </div>
                </div>
            </nav>
            <div className="w-full h-16"></div>
        </>
    )
}

export default Navbar
