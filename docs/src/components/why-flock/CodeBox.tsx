const CodeBox = () => {
    return (
        <div className="rounded-2xl shadow-xl p-4 overflow-hidden bg-black">
            <div className="flex justify-between pt-1 pl-2 md:pl-4 pr-4 items-center">
                <div className="flex justify-start space-x-2">
                    <div className="w-3 h-3 bg-gray-500 rounded-full"></div>
                    <div className="w-3 h-3 bg-gray-500 rounded-full"></div>
                    <div className="w-3 h-3 bg-gray-500 rounded-full"></div>
                </div>
            </div>
            <div className="flex">
                <div className="p-2 md:p-4 text-gray-400 text-xs md:text-sm font-mono h-full">
                    <div>1</div>
                    <div>2</div>
                    <div>3</div>
                    <div>4</div>
                    <div>5</div>
                    <div>6</div>
                    <br />
                    <div>7</div>
                    <div>8</div>
                    <div>9</div>
                    <div>10</div>
                    <div>11</div>
                </div>
                <div>
                <pre className="p-2 md:p-4 text-white text-xs md:text-sm font-mono bg-transparent m-auto">
                    <code>
                        <span className="text-green-400">&gt;</span>{" "}
                        <span className="text-blue-400 font-bold">SELECT</span><br />
                        {"    "}<span className="text-yellow-300">paper_id</span>,<br />
                        {"    "}<span className="text-yellow-300">paper_title</span>,<br />
                        {"    "}<span className="text-blue-400 font-bold">llm_complete</span>(<br />
                        {"        "}<span className="text-green-300">{"{"}</span><span className="text-yellow-300">'model_name'</span>: <span className="test-white-100">'gpt-4o'</span><span className="text-green-300">{"}"}</span>,<br />
                            {"        "}<span className="text-green-300">{"{"}</span><span className="text-yellow-300">'prompt'</span>: <span className="test-white-100">'Extract the top 3 <br />keywords from the abstract.'</span><span className="text-green-300">{"}"}</span>,<br />
                        {"        "}<span className="text-green-300">{"{"}</span><span className="text-yellow-300">'abstract'</span>: <span className="test-white-100">abstract</span><span className="text-green-300">{"}"}</span><br />
                        {"    "}) <span className="text-green-300">AS</span> <span className="text-yellow-300">extracted_keywords</span><br />
                        {"  "}<span className="text-blue-400 font-bold">FROM</span> <span className="text-yellow-300">research_papers</span><br />
                        {"  "}<span className="text-blue-400 font-bold">WHERE</span> <span className="text-yellow-300">publication_year</span> <span className="text-green-300">&gt;</span> <span className="text-yellow-300">2018</span><br />
                        {"  "}<span className="text-blue-400 font-bold">LIMIT</span> <span className="text-yellow-300">5</span>
                        {"  "}<span className="text-gray-500">;</span>
                    </code>
                </pre>
                </div>
            </div>
        </div>
    )
}

export default CodeBox