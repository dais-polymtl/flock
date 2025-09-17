import {Reveal} from "@site/src/components/Reveal";
import {useState} from "react";
import {FaClipboard} from "react-icons/fa";

const CodeBox = () => {

    const [copied, setCopied] = useState(false);

    const copyToClipboard = () => {
        const code = "INSTALL flock FROM community; LOAD flock;";
        navigator.clipboard.writeText(code);
        setCopied(true);
        setTimeout(() => {
            setCopied(false);
        }, 2000);
    }

    return (
        <Reveal>
            <div className="rounded-2xl shadow-xl bg-black mb-8 z-10">
                <div className="flex justify-between pt-4 pl-4 mb-4 pr-4 items-center">
                    <Reveal>
                        <div className="flex justify-start space-x-2">
                            <div className="w-3 h-3 bg-gray-500 rounded-full"></div>
                            <div className="w-3 h-3 bg-gray-500 rounded-full"></div>
                            <div className="w-3 h-3 bg-gray-500 rounded-full"></div>
                        </div>
                    </Reveal>
                    {/* copy to clipboard */}
                    <div>
                        <Reveal>
                            <div
                                className="flex items-center space-x-2 cursor-pointer hover:text-[#FF9128] text-gray-500"
                                onClick={copyToClipboard}>
                                {copied &&
                                    <span className="hover:text-gray-500 text-gray-500 text-[10px]">copied !</span>}
                                <FaClipboard/>
                            </div>
                        </Reveal>
                    </div>
                </div>
                <div className="flex">
                    <div className="w-fit p-6 text-gray-400 text-xs md:text-sm font-mono">
                        <Reveal>
                            <div>1</div>
                            <div>2</div>
                        </Reveal>
                    </div>
                    <Reveal>
                        <pre
                            className="py-6 px-4 bg-transparent text-white text-xs md:text-sm font-mono overflow-x-auto">
                            <code className="language-sql">
                                <span className="text-green-400">&gt;</span>{" "}
                                <span className="text-blue-400 font-bold">INSTALL</span>{" "}
                                <span className="text-yellow-300">flock</span>{" "}
                                <span className="text-blue-400 font-bold">FROM</span>{" "}
                                <span className="text-green-300">community</span>
                                <span className="text-gray-500">;</span>
                                <br/>
                                <span className="text-green-400">&gt;</span>{" "}
                                <span className="text-blue-400 font-bold">LOAD</span>{" "}
                                <span className="text-yellow-300">flock</span>
                                <span className="text-gray-500">;</span>
                            </code>
                        </pre>
                    </Reveal>
                </div>
            </div>
        </Reveal>
    )
}

export default CodeBox;