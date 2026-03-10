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
                    <div>12</div>
                    <div>13</div>
                    <div>14</div>
                    <div>15</div>
                    <div>16</div>
                    <div>17</div>
                </div>
                <div>
                    <pre className="p-2 md:p-4 text-white text-xs md:text-sm font-mono bg-transparent m-auto">
                        <code>
                            <span className="text-gray-500">-- 1) Create a model</span><br />
                            <span className="text-blue-400 font-bold">CREATE MODEL</span>(<br />
                            {"  "}<span className="text-yellow-300">'product_summarizer'</span>,<br />
                            {"  "}<span className="text-yellow-300">'gpt-4o'</span>,<br />
                            {"  "}<span className="text-yellow-300">'openai'</span><br />
                            );<br />
                            <br />
                            <span className="text-gray-500">-- 2) Call it from SQL</span><br />
                            <span className="text-blue-400 font-bold">SELECT</span><br />
                            {"  "}<span className="text-yellow-300">product_id</span>,<br />
                            {"  "}<span className="text-yellow-300">name</span>,<br />
                            {"  "}<span className="text-blue-400 font-bold">llm_complete</span>(<br />
                            {"    "}<span className="text-green-300">{"{"}</span><span className="text-yellow-300">'model_name'</span>: <span className="text-white">'product_summarizer'</span><span className="text-green-300">{"}"}</span>,<br />
                            {"    "}<span className="text-green-300">{"{"}</span>
                            <span className="text-yellow-300">'prompt'</span>: <span className="text-white">'Summarize this product in one sentence.'</span>,<br />
                            {"      "}<span className="text-yellow-300">'context_columns'</span>: [<span className="text-green-300">{"{"}</span><span className="text-yellow-300">'data'</span>: name<span className="text-green-300">{"}"}</span>]
                            <span className="text-green-300">{"}"}</span><br />
                            {"  "}) <span className="text-green-300">AS</span> <span className="text-yellow-300">short_description</span><br />
                            <span className="text-blue-400 font-bold">FROM</span> <span className="text-yellow-300">products</span><br />
                            <span className="text-blue-400 font-bold">LIMIT</span> <span className="text-yellow-300">3</span><span className="text-gray-500">;</span>
                        </code>
                    </pre>
                </div>
            </div>
        </div>
    );
};

export default CodeBox;