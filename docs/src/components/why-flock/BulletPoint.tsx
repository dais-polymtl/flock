import { Reveal } from "@site/src/components/Reveal";

const BulletPoint = ({ children }: { children: React.ReactNode }) => {
    return (
        <Reveal>
            <div className="flex md:flex-row flex-col items-center container md:gap-8 gap-2 p-4">
                <div className="md:w-1 md:h-10 bg-[#FF9128] w-8 h-1"></div>
                <p className="m-auto">{children}</p>
            </div>
        </Reveal>
    );
}

export default BulletPoint;