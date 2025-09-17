const TableBox = () => {
    return (
        <table className="!table-auto rounded-xl p-2 text-xs md:text-sm bg-black">
                <thead className="">
                    <tr>
                        <th>ID</th>
                        <th>Paper Title</th>
                        <th>Extracted Keywords</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td>1</td>
                        <td>Innovations in Biotechnology</td>
                        <td>Biotechnology, Genetic Engineering, CRISPR</td>
                    </tr>
                    <tr>
                        <td>2</td>
                        <td>The Rise of Autonomous Vehicles</td>
                        <td>Autonomous Vehicles, AI, Self-driving Cars</td>
                    </tr>
                    <tr>
                        <td>3</td>
                        <td>Exploring Renewable Energy Solutions</td>
                        <td>Renewable Energy, Solar Power, Wind Turbines</td>
                    </tr>
                    <tr>
                        <td>4</td>
                        <td>Understanding Blockchain Technology</td>
                        <td>Blockchain, Cryptocurrencies, Decentralization</td>
                    </tr>
                </tbody>
            </table>
    )
}

export default TableBox