import type { ReactNode, SVGProps } from "react";

// Iconos propios en SVG inline: sin dependencia de una libreria ni de red
// (la app tiene que funcionar offline). Trazo en currentColor, 24x24.

type IconProps = SVGProps<SVGSVGElement> & { size?: number };

function Icon({ size = 18, children, ...rest }: IconProps & { children: ReactNode }) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth={1.8}
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
      focusable="false"
      {...rest}
    >
      {children}
    </svg>
  );
}

export const SearchIcon = (p: IconProps) => (
  <Icon {...p}>
    <circle cx="11" cy="11" r="7" />
    <path d="m20 20-3.5-3.5" />
  </Icon>
);

export const PlusIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M12 5v14M5 12h14" />
  </Icon>
);

export const TrashIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13M10 11v6M14 11v6" />
  </Icon>
);

export const EyeIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12Z" />
    <circle cx="12" cy="12" r="3" />
  </Icon>
);

export const DownloadIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M12 4v11M7 10l5 5 5-5M5 20h14" />
  </Icon>
);

export const UploadIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M12 16V5M7 10l5-5 5 5M5 20h14" />
  </Icon>
);

export const PlayIcon = (p: IconProps) => (
  <Icon {...p}>
    <circle cx="12" cy="12" r="9" />
    <path d="m10 8.5 5.5 3.5-5.5 3.5Z" />
  </Icon>
);

export const RefreshIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M20 11a8 8 0 0 0-14.5-4.5L4 8M4 4v4h4M4 13a8 8 0 0 0 14.5 4.5L20 16M20 20v-4h-4" />
  </Icon>
);

export const ArrowLeftIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M19 12H5M11 6l-6 6 6 6" />
  </Icon>
);

export const CheckIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="m5 12.5 4.5 4.5L19 7.5" />
  </Icon>
);

export const ClockIcon = (p: IconProps) => (
  <Icon {...p}>
    <circle cx="12" cy="13" r="8" />
    <path d="M12 9v4l2.5 2.5M9 2h6" />
  </Icon>
);

export const FileAudioIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M14 3H6v18h12V7Z" />
    <path d="M14 3v4h4M9 13v3M12 11v7M15 13v3" />
  </Icon>
);

export const NoFileIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M14 3H6v18h12V7Z" />
    <path d="M14 3v4h4M3 3l18 18" />
  </Icon>
);

export const AlertIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M12 3 2 20h20Z" />
    <path d="M12 10v4M12 17h.01" />
  </Icon>
);

export const InfoIcon = (p: IconProps) => (
  <Icon {...p}>
    <circle cx="12" cy="12" r="9" />
    <path d="M12 11v5M12 8h.01" />
  </Icon>
);

export const ShieldIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M12 3 4 6v6c0 4.5 3.4 8 8 9 4.6-1 8-4.5 8-9V6Z" />
    <path d="m9 12 2 2 4-4" />
  </Icon>
);

export const ArchiveIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M3 5h18v4H3ZM5 9v10h14V9M10 13h4" />
  </Icon>
);

export const HourglassIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M6 3h12M6 21h12M7 3c0 5 5 6 5 9s-5 4-5 9M17 3c0 5-5 6-5 9s5 4 5 9" />
  </Icon>
);

export const TempleIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M3 9 12 4l9 5M4 9h16M6 9v9M10 9v9M14 9v9M18 9v9M3 20h18" />
  </Icon>
);

export const BookIcon = (p: IconProps) => (
  <Icon {...p}>
    <path d="M4 5c3-1.5 5.5-1.5 8 0v14c-2.5-1.5-5-1.5-8 0ZM12 5c2.5-1.5 5-1.5 8 0v14c-3-1.5-5.5-1.5-8 0" />
  </Icon>
);
