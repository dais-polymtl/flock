import React from "react";

interface RevealProps {
    children: React.ReactNode;
    width?: "fit-content" | "100%";
}

/** Passthrough wrapper kept for call-site compatibility; scroll animations removed. */
export const Reveal: React.FC<RevealProps> = ({ children }) => <>{children}</>;
