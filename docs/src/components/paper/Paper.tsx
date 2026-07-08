import {useState} from "react";
import styles from "@site/src/css/style";
import {bibtex} from "@site/src/constants";

const Paper: React.FC = () => {
    const [copied, setCopied] = useState(false);

    const copyBibtex = async () => {
        try {
            await navigator.clipboard.writeText(bibtex);
            setCopied(true);
            setTimeout(() => setCopied(false), 2000);
        } catch {
            setCopied(false);
        }
    };

    return (
        <section id="paper" className={`${styles.paddingY} w-full`}>
            <h2 className={`${styles.heading2} text-center mb-8`}>Paper</h2>
            <div className="max-w-3xl mx-auto flex flex-col gap-6">
                <div className="text-center space-y-2">
                    <p className="text-xl font-semibold text-black dark:text-white">
                        Beyond Quacking: Deep Integration of Language Models and RAG into DuckDB
                    </p>
                    <p className="text-gray-600 dark:text-gray-300">
                        Anas Dorbani, Sunny Yasser, Jimmy Lin, Amine Mhedhbi
                    </p>
                    <p className="text-gray-500 dark:text-gray-400">
                        Proc. VLDB Endow. 2025, Vol. 18, No. 12
                    </p>
                    <p>
                        <a
                            href="https://doi.org/10.14778/3750601.3750685"
                            target="_blank"
                            rel="noopener noreferrer"
                            className="text-[#FF9128] hover:underline"
                        >
                            https://doi.org/10.14778/3750601.3750685
                        </a>
                    </p>
                </div>
                <div className="relative">
                    <pre className="bg-gray-100 dark:bg-[#1e1e1e] rounded-lg p-4 pt-12 overflow-x-auto text-left text-sm leading-relaxed border border-gray-200 dark:border-gray-700">
                        <code>{bibtex}</code>
                    </pre>
                    <button
                        type="button"
                        onClick={copyBibtex}
                        className={`${styles.animate} absolute top-3 right-3 px-3 py-1.5 text-sm rounded-md bg-[#FF9128] text-white hover:bg-[#e07d1a]`}
                    >
                        {copied ? "Copied" : "Copy"}
                    </button>
                </div>
            </div>
        </section>
    );
};

export default Paper;
