import styles from "@site/src/css/style";

const Button = ({ children, className, href }: { children: React.ReactNode; className?: string, href?: string }) => {
    return (
        <a href={href} className={`${className} px-8 py-3 font-semibold rounded-lg flex items-center gap-2 ${styles.animate}`}>
            {children}
        </a>
    );
}

export default Button;
