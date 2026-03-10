const TableBox = () => {
    return (
        <table className="!table-auto rounded-xl p-2 text-xs md:text-sm bg-black">
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Product</th>
                    <th>Short Description (from LLM)</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>1</td>
                    <td>Wireless Headphones</td>
                    <td>Comfortable Bluetooth headphones with clear sound and long battery life.</td>
                </tr>
                <tr>
                    <td>2</td>
                    <td>Gaming Laptop</td>
                    <td>High‑performance laptop designed for modern games and creative workloads.</td>
                </tr>
                <tr>
                    <td>3</td>
                    <td>Smart Watch</td>
                    <td>Everyday smartwatch that tracks activity, notifications, and heart rate.</td>
                </tr>
            </tbody>
        </table>
    );
};

export default TableBox;