import { FaFilter } from "react-icons/fa6";
import { BsDatabaseFillGear, BsFillFileEarmarkTextFill } from "react-icons/bs";
import { AmineMhedhbi, AnasDorbani, SunnyYasser } from "@site/static";

export const navLinks = [
    {
        id: "features",
        title: "Features",
    },
    {
        id: "why-flockmtl",
        title: "Why FlockMTL?",
    },
    {
        id: "getting-started",
        title: "Getting Started",
    },
    {
        id: "team",
        title: "Team",
    },
];

export const features = [
    {
        id: "feature-1",
        icon: BsDatabaseFillGear,
        title: "Administrative Views for Managing Prompts and Models",
        content:
            "Treat models and prompts as resources similar to tables. You can CREATE PROMPT(s) and MODEL(s) and iterate on them without making changes to SQL queries.",
    },
    {
        id: "feature-2",
        icon: FaFilter,
        title: "Scalar / Map Functions for Tuple and Passage Transformation",
        content:
            "Map tuples or text passages to new text, boolean, or JSON values using LLMs to support various types of semantic analysis such as extraction and classification. ",
    },
    {
        id: "feature-3",
        icon: BsFillFileEarmarkTextFill,
        title: "Aggregate / Reduce Functions to Reduce over Tuples and Passages",
        content:
            "Use Aggregate / Reduce functions to reduce tuples or text passages using LLMs to support various types of semantic analysis such as reranking and summarization.",
    },
];


export const whyFlockMTL = [
    {
        id: "why-flockmtl-1",
        content: "Declarative LLM integration within SQL",
    },
    {
        id: "why-flockmtl-2",
        content: "Transparently adds lower-level optimizations, e.g., batching and caching",
    },
    {
        id: "why-flockmtl-3",
        content: "Add tabular understanding capabilities to your relational databases",
    }
];

export const team = [
    {
        id: "team-member-1",
        name: "Anas Dorbani",
        role: "PhD Student",
        company: "Data Systems Group, Polytechnique Montréal",
        img: AnasDorbani,
        github: "https://github.com/anasdorbani",
        linkedin: "https://linkedin.com/in/anasdorbani",
        X: "https://x.com/anasdorbani",
    },
    {
        id: "team-member-2",
        name: "Sunny Yasser",
        role: "PhD Student",
        company: "Data Systems Group, Polytechnique Montréal",
        img: SunnyYasser,
        github: "https://github.com/SunnyYasser",
        linkedin: "https://linkedin.com/in/sunny-yasser/",
        X: "",
    },
    {
        id: "team-member-3",
        name: "Amine Mhedhbi",
        role: "Faculty",
        company: "Polytechnique Montréal",
        img: AmineMhedhbi,
        github: "https://github.com/queryproc/",
        linkedin: "https://linkedin.com/in/queryproc/",
        X: "https://x.com/dsg_polymtl",
    }
];