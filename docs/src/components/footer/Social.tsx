import { IconType } from "react-icons/lib";
import styles from "@site/src/css/style";

const Social = ({ Icon, href }: { Icon: IconType, href?: string }) => {
    return (
        <a href={href} className={`${styles.animate} w-14 h-14 bg-[#FF9128] rounded-full text-2xl p-2 flex items-center text-white justify-center hover:bg-[#ffbb7c] cursor-pointer`}>
            <Icon />
        </a>
    )
}

export default Social;
