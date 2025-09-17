import styles from '@site/src/css/style';
import { FlockHorizontal, FlockHorizontalDark } from '@site/static';

const ThemeLogo = () => {
    return (
        <a href='/flock/' className={styles.animate}>
            <img
                src={FlockHorizontalDark}
                alt="Flock Logo Dark"
                width={150}
                height={50}
                className='hidden dark:block'
            />
            <img
                src={FlockHorizontal}
                alt="Flock Logo"
                width={150}
                height={50}
                className='block dark:hidden'
            />
        </a>
    );
};

export default ThemeLogo;
