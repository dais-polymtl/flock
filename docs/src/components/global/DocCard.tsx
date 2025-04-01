import React from "react";
import type { IconType } from "react-icons/lib";

export default function DocCard(props: { Icon: IconType; title: string; link: string }) {
    const { Icon, title, link } = props;

    return (
        <>
            <div onClick={() => window.open(link, "_self")}
                className="my-2 flex gap-2 cursor-pointer rounded-2xl items-center text-lg font-bold border-solid border-[1px] border-[#FF9128] p-4 hover:shadow-[0_0_10px_#FF9128] hover:shadow-orange-500/50 transition-all duration-300 ease-in-out">
                <Icon className="text-[#FF9128] text-3xl rounded-lg border-solid border-[1px] border-[#FF9128] p-1" />
                {title}
            </div>
        </>
    );
}