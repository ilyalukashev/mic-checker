"use client";

import { useCallback, useRef, useState } from "react";

interface ImageItem {
  id: string;
  file: File;
  preview: string;
  data: string;
  mediaType: string;
}

interface Props {
  images: ImageItem[];
  onImagesAdded: (images: ImageItem[]) => void;
  onRemoveImage: (id: string) => void;
}

const ACCEPTED_TYPES = ["image/jpeg", "image/png", "image/webp", "image/gif"];
const MAX_SIZE_MB = 20;

async function fileToBase64(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve((reader.result as string).split(",")[1]);
    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}

function ImageThumb({
  img,
  onRemove,
}: {
  img: ImageItem;
  onRemove: () => void;
}) {
  const [searching, setSearching] = useState(false);

  const handleGoogleSearch = async (e: React.MouseEvent) => {
    e.stopPropagation();
    setSearching(true);
    try {
      const res = await fetch("/api/reverse-search", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ data: img.data, mediaType: img.mediaType }),
      });
      const { url } = await res.json();
      if (url) window.open(url, "_blank");
    } catch {
      // fallback — open Google Images manually
      window.open("https://images.google.com", "_blank");
    } finally {
      setSearching(false);
    }
  };

  return (
    <div
      className="relative group aspect-square rounded-lg overflow-hidden"
      style={{ backgroundColor: "#0d0d0d" }}
    >
      {/* eslint-disable-next-line @next/next/no-img-element */}
      <img src={img.preview} alt={img.file.name} className="w-full h-full object-cover" />

      {/* Overlay on hover */}
      <div
        className="absolute inset-0 opacity-0 group-hover:opacity-100 transition-opacity flex flex-col items-center justify-center gap-1.5"
        style={{ backgroundColor: "rgba(0,0,0,0.6)" }}
      >
        {/* Google reverse search button */}
        <button
          onClick={handleGoogleSearch}
          disabled={searching}
          title="Search on Google Images"
          className="flex items-center gap-1 px-2 py-1 rounded text-xs font-medium transition-colors disabled:opacity-50"
          style={{ backgroundColor: "#C9A96E", color: "#000" }}
        >
          {searching ? (
            <svg className="animate-spin w-3 h-3" fill="none" viewBox="0 0 24 24">
              <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
              <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z" />
            </svg>
          ) : (
            <svg className="w-3 h-3" viewBox="0 0 24 24" fill="currentColor">
              <path d="M21.35 11.1h-9.17v2.73h6.51c-.33 3.81-3.5 5.44-6.5 5.44C8.36 19.27 5 16.25 5 12c0-4.1 3.2-7.27 7.2-7.27 3.09 0 4.9 1.97 4.9 1.97L19 4.72S16.56 2 12.1 2C6.42 2 2.03 6.8 2.03 12c0 5.05 4.13 10 10.22 10 5.35 0 9.25-3.67 9.25-9.09 0-1.15-.15-1.81-.15-1.81z" />
            </svg>
          )}
          {searching ? "Searching…" : "Google"}
        </button>

        {/* Remove button */}
        <button
          onClick={(e) => { e.stopPropagation(); onRemove(); }}
          title="Remove"
          className="flex items-center gap-1 px-2 py-1 rounded text-xs transition-colors"
          style={{ backgroundColor: "rgba(255,255,255,0.1)", color: "#aaa" }}
        >
          <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
          </svg>
          Remove
        </button>
      </div>
    </div>
  );
}

export default function ImageUploader({ images, onImagesAdded, onRemoveImage }: Props) {
  const inputRef = useRef<HTMLInputElement>(null);

  const processFiles = useCallback(
    async (files: FileList | File[]) => {
      const valid = Array.from(files).filter(
        (f) => ACCEPTED_TYPES.includes(f.type) && f.size <= MAX_SIZE_MB * 1024 * 1024
      );
      const items: ImageItem[] = await Promise.all(
        valid.map(async (file) => ({
          id: `${file.name}-${Date.now()}-${Math.random()}`,
          file,
          preview: URL.createObjectURL(file),
          data: await fileToBase64(file),
          mediaType: file.type,
        }))
      );
      if (items.length > 0) onImagesAdded(items);
    },
    [onImagesAdded]
  );

  const handleDrop = useCallback(
    (e: React.DragEvent) => { e.preventDefault(); processFiles(e.dataTransfer.files); },
    [processFiles]
  );

  const handleChange = useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) => {
      if (e.target.files) processFiles(e.target.files);
      e.target.value = "";
    },
    [processFiles]
  );

  return (
    <div>
      <div
        onDrop={handleDrop}
        onDragOver={(e) => e.preventDefault()}
        onClick={() => inputRef.current?.click()}
        className="rounded-xl p-10 text-center cursor-pointer transition-all"
        style={{ border: "1px dashed #222" }}
        onMouseEnter={e => (e.currentTarget.style.borderColor = "#444")}
        onMouseLeave={e => (e.currentTarget.style.borderColor = "#222")}
      >
        <svg
          className="w-7 h-7 mx-auto mb-3"
          style={{ color: "#333" }}
          fill="none" stroke="currentColor" viewBox="0 0 24 24"
        >
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5}
            d="M3 16.5v2.25A2.25 2.25 0 005.25 21h13.5A2.25 2.25 0 0021 18.75V16.5m-13.5-9L12 3m0 0l4.5 4.5M12 3v13.5" />
        </svg>
        <p className="text-sm" style={{ color: "#555" }}>
          <span style={{ color: "#888" }}>Click to upload</span> or drag & drop
        </p>
        <p className="text-xs mt-1" style={{ color: "#333" }}>JPEG, PNG, WebP — up to {MAX_SIZE_MB} MB</p>
        <input
          ref={inputRef}
          type="file"
          accept={ACCEPTED_TYPES.join(",")}
          multiple
          onChange={handleChange}
          className="hidden"
        />
      </div>

      {images.length > 0 && (
        <>
          <p className="text-xs mt-3 mb-2 text-center" style={{ color: "#444" }}>
            Hover over a photo to search on Google Images
          </p>
          <div className="grid grid-cols-4 gap-2">
            {images.map((img) => (
              <ImageThumb
                key={img.id}
                img={img}
                onRemove={() => onRemoveImage(img.id)}
              />
            ))}
            <div
              onClick={() => inputRef.current?.click()}
              className="aspect-square rounded-lg flex items-center justify-center cursor-pointer transition-all"
              style={{ border: "1px dashed #222" }}
              onMouseEnter={e => (e.currentTarget.style.borderColor = "#444")}
              onMouseLeave={e => (e.currentTarget.style.borderColor = "#222")}
            >
              <svg className="w-5 h-5" style={{ color: "#333" }} fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M12 4v16m8-8H4" />
              </svg>
            </div>
          </div>
        </>
      )}
    </div>
  );
}
