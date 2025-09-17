import React, { useState } from "react";
import { motion } from "framer-motion";
import CodeBox from "./CodeBox";
import TableBox from "./TableBox";

const AnimatedCodeBlockToTable = () => {
    const [showTable, setShowTable] = useState(false);

    return (
        <div className="container mx-auto py-12 px-6 text-white">
            {/* Conditional Rendering */}
            <div className="absolute inset-0 flex justify-center items-center z-0">
                <div className="w-96 h-96 bg-orange-500 opacity-30 rounded-full"></div>
                {/* Middle Circle */}
                <div className="absolute w-72 h-72 bg-orange-500 opacity-40 rounded-full"></div>
                {/* Inner Circle */}
                <div className="absolute w-48 h-48 bg-orange-500 opacity-50 rounded-full"></div>
            </div>
            <div className="flex justify-center items-center">
                {!showTable ? (
                    <motion.div
                        key="code"
                        initial={{ opacity: 0, scale: 0 }}
                        animate={{
                            opacity: [0, 1, 1, 1],
                            scale: [0, 1, 1, 0],
                            transition: {
                                duration: 6,
                                ease: "easeOut",
                                times: [0, 0.1, 0.9, 1]
                            },
                        }}
                        onAnimationComplete={() => setShowTable(true)}
                        className="flex-1"
                    >
                        <CodeBox />
                    </motion.div>
                ) : (
                    <motion.div
                        key="table"
                        initial={{ opacity: 0, scale: 0 }}
                        animate={{
                            opacity: [0, 1, 1, 1],
                            scale: [0, 1, 1, 0],
                            transition: {
                                duration: 6,
                                ease: "easeOut",
                                times: [0, 0.1, 0.9, 1]
                            },
                        }}
                        onAnimationComplete={() => setShowTable(false)}
                        className="flex-1"
                    >
                        <TableBox />
                    </motion.div>
                )}
            </div>
        </div>
    );
};

export default AnimatedCodeBlockToTable;
