"use client";

import { useCallback, useRef } from "react";

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
        className="rounded-xl p-10 text-center cursor-pointer transition-all group"
        style={{ border: "1px dashed #222" }}
        onMouseEnter={e => (e.currentTarget.style.borderColor = "#444")}
        onMouseLeave={e => (e.currentTarget.style.borderColor = "#222")}
      >
        <svg
          className="w-7 h-7 mx-auto mb-3 transition-colors"
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
        <div className="mt-3 grid grid-cols-4 gap-2">
          {images.map((img) => (
            <div
              key={img.id}
              className="relative group aspect-square rounded-lg overflow-hidden"
              style={{ backgroundColor: "#0d0d0d" }}
            >
              {/* eslint-disable-next-line @next/next/no-img-element */}
              <img src={img.preview} alt={img.file.name} className="w-full h-full object-cover" />
              <button
                onClick={(e) => { e.stopPropagation(); onRemoveImage(img.id); }}
                className="absolute top-1 right-1 rounded-full p-0.5 opacity-0 group-hover:opacity-100 transition-opacity"
                style={{ backgroundColor: "rgba(0,0,0,0.7)" }}
              >
                <svg className="w-3 h-3" style={{ color: "#aaa" }} fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                </svg>
              </button>
            </div>
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
      )}
    </div>
  );
}
