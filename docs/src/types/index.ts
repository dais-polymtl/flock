import { IconType } from "react-icons/lib";

export interface FeatredCardProps {
    Icon: IconType;
    title: string;
    content: string;
    index: number;
}

export interface ButtonProps {
    styles?: string;
}

export interface FeedBackProps {
    content: string;
    title: string;
    name: string;
    img: string | any;
}