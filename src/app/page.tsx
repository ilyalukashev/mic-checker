"use client";

import { useState, useCallback } from "react";
import ImageUploader from "@/components/ImageUploader";
import AnalysisResult from "@/components/AnalysisResult";

export interface AnalysisData {
  identified_model: string;
  authenticity_verdict: string;
  confidence_percentage: number;
  summary: string;
  positive_indicators: string[];
  red_flags: string[];
  inconclusive_aspects: string[];
  recommendations: string[];
}

interface ImageItem {
  id: string;
  file: File;
  preview: string;
  data: string;
  mediaType: string;
}

const PHOTO_TIPS = [
  { label: "Full front view",        optional: false },
  { label: "Back with serial number",optional: false },
  { label: "Badge close-up",         optional: false },
  { label: "Headgrille mesh",        optional: false },
  { label: "XLR connector base",     optional: false },
  { label: "Switches (U87)",         optional: false },
  { label: "PCB — front side",       optional: false },
  { label: "PCB — back side",        optional: false },
  { label: "Capsule",                optional: true  },
];

export default function Home() {
  const [images, setImages] = useState<ImageItem[]>([]);
  const [analysis, setAnalysis] = useState<AnalysisData | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleImagesAdded = useCallback((newImages: ImageItem[]) => {
    setImages((prev) => [...prev, ...newImages]);
    setAnalysis(null);
    setError(null);
  }, []);

  const handleRemoveImage = useCallback((id: string) => {
    setImages((prev) => {
      const item = prev.find((i) => i.id === id);
      if (item) URL.revokeObjectURL(item.preview);
      return prev.filter((i) => i.id !== id);
    });
    setAnalysis(null);
  }, []);

  const handleAnalyze = async () => {
    if (images.length === 0) return;
    setLoading(true);
    setError(null);
    setAnalysis(null);
    try {
      const res = await fetch("/api/analyze", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ images: images.map((i) => ({ data: i.data, mediaType: i.mediaType })) }),
      });
      if (!res.ok) {
        const err = await res.json();
        throw new Error(err.error || "Analysis failed");
      }
      setAnalysis(await res.json());
    } catch (err) {
      setError(err instanceof Error ? err.message : "Something went wrong");
    } finally {
      setLoading(false);
    }
  };

  const handleReset = () => {
    images.forEach((i) => URL.revokeObjectURL(i.preview));
    setImages([]);
    setAnalysis(null);
    setError(null);
  };

  return (
    <main className="max-w-xl mx-auto px-5 py-16 text-center">

      {/* ── Header ── */}
      <div className="mb-10">
        <p className="text-xs font-mono tracking-[0.25em] uppercase mb-5" style={{ color: "#C9A96E" }}>
          U87 · TLM103
        </p>
        <h1 className="text-4xl font-semibold text-white tracking-tight mb-2">
          Mic Authenticator
        </h1>

        {/* gold underline */}
        <div className="flex items-center justify-center gap-3 mb-4">
          <div className="h-px w-12" style={{ backgroundColor: "#C9A96E", opacity: 0.4 }} />
          <div className="w-1 h-1 rounded-full" style={{ backgroundColor: "#C9A96E" }} />
          <div className="h-px w-12" style={{ backgroundColor: "#C9A96E", opacity: 0.4 }} />
        </div>

        <p className="text-xs mb-5" style={{ color: "#555" }}>
          by{" "}
          <a
            href="https://www.facebook.com/ilya.lukashev"
            target="_blank"
            rel="noopener noreferrer"
            style={{ color: "#888" }}
            onMouseEnter={e => (e.currentTarget.style.color = "#C9A96E")}
            onMouseLeave={e => (e.currentTarget.style.color = "#888")}
          >
            Ilia Lukashev
          </a>
        </p>

        <p className="text-sm leading-relaxed mx-auto max-w-xs" style={{ color: "#666" }}>
          Upload photos of your microphone to check authenticity based on visual markers.
        </p>
      </div>

      {/* ── Recommended shots ── */}
      <div className="mb-10">
        {/* section label with lines */}
        <div className="flex items-center gap-3 mb-5">
          <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
          <p className="text-xs uppercase tracking-[0.2em] font-mono" style={{ color: "#444" }}>
            Recommended shots
          </p>
          <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
        </div>

        <div className="grid grid-cols-2 gap-x-8 gap-y-2.5 text-left max-w-sm mx-auto">
          {PHOTO_TIPS.map((tip) => (
            <div key={tip.label} className="flex items-center gap-2.5">
              <span
                className="w-1 h-1 rounded-full flex-shrink-0"
                style={{ backgroundColor: tip.optional ? "#2a2a2a" : "#C9A96E" }}
              />
              <span className="text-sm" style={{ color: tip.optional ? "#3a3a3a" : "#888" }}>
                {tip.label}
                {tip.optional && <span style={{ color: "#3a3a3a" }} className="ml-1">— optional</span>}
              </span>
            </div>
          ))}
        </div>
      </div>

      {/* ── Upload ── */}
      <div className="flex items-center gap-3 mb-5">
        <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
        <p className="text-xs uppercase tracking-[0.2em] font-mono" style={{ color: "#444" }}>
          Upload photos
        </p>
        <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
      </div>

      <ImageUploader
        images={images}
        onImagesAdded={handleImagesAdded}
        onRemoveImage={handleRemoveImage}
      />

      {/* Actions */}
      {images.length > 0 && !analysis && (
        <div className="mt-5 flex gap-3">
          <button
            onClick={handleAnalyze}
            disabled={loading}
            className="flex-1 font-medium py-2.5 px-5 rounded-lg text-sm transition-all disabled:opacity-40 disabled:cursor-not-allowed"
            style={{ backgroundColor: "#C9A96E", color: "#000" }}
            onMouseEnter={e => { if (!loading) e.currentTarget.style.backgroundColor = "#d4b87e"; }}
            onMouseLeave={e => { e.currentTarget.style.backgroundColor = "#C9A96E"; }}
          >
            {loading ? (
              <span className="flex items-center justify-center gap-2">
                <svg className="animate-spin w-3.5 h-3.5" fill="none" viewBox="0 0 24 24">
                  <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
                  <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z" />
                </svg>
                Analyzing…
              </span>
            ) : (
              `Analyze ${images.length} ${images.length > 1 ? "images" : "image"}`
            )}
          </button>
          <button
            onClick={handleReset}
            className="px-4 py-2.5 rounded-lg text-sm transition-colors"
            style={{ border: "1px solid #222", color: "#555" }}
            onMouseEnter={e => { e.currentTarget.style.color = "#999"; e.currentTarget.style.borderColor = "#444"; }}
            onMouseLeave={e => { e.currentTarget.style.color = "#555"; e.currentTarget.style.borderColor = "#222"; }}
          >
            Clear
          </button>
        </div>
      )}

      {/* Error */}
      {error && (
        <div className="mt-5 rounded-lg p-4 text-sm text-left" style={{ border: "1px solid #3a1515", backgroundColor: "#1a0808", color: "#cc6666" }}>
          {error}
        </div>
      )}

      {/* Results */}
      {analysis && (
        <div className="mt-10 text-left">
          {/* section label */}
          <div className="flex items-center gap-3 mb-5">
            <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
            <p className="text-xs uppercase tracking-[0.2em] font-mono" style={{ color: "#444" }}>Analysis</p>
            <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
          </div>
          <AnalysisResult data={analysis} />
          <button
            onClick={handleReset}
            className="mt-6 w-full py-2.5 rounded-lg text-sm transition-colors"
            style={{ border: "1px solid #222", color: "#555" }}
            onMouseEnter={e => { e.currentTarget.style.color = "#999"; e.currentTarget.style.borderColor = "#444"; }}
            onMouseLeave={e => { e.currentTarget.style.color = "#555"; e.currentTarget.style.borderColor = "#222"; }}
          >
            Start over
          </button>
        </div>
      )}

      {/* ── Footer ── */}
      <footer className="mt-16">
        <div className="flex items-center gap-3 mb-6">
          <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
          <div className="w-1 h-1 rounded-full" style={{ backgroundColor: "#2a2a2a" }} />
          <div className="flex-1 h-px" style={{ backgroundColor: "#1a1a1a" }} />
        </div>

        <div className="flex flex-wrap items-center justify-center gap-x-6 gap-y-3 mb-5">
          <a href="https://www.lukashev.studio" target="_blank" rel="noopener noreferrer"
            className="flex items-center gap-1.5 text-sm transition-colors"
            style={{ color: "#555" }}
            onMouseEnter={e => (e.currentTarget.style.color = "#999")}
            onMouseLeave={e => (e.currentTarget.style.color = "#555")}
          >
            <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5}
                d="M12 21a9.004 9.004 0 008.716-6.747M12 21a9.004 9.004 0 01-8.716-6.747M12 21c2.485 0 4.5-4.03 4.5-9S14.485 3 12 3m0 18c-2.485 0-4.5-4.03-4.5-9S9.515 3 12 3m0 0a8.997 8.997 0 017.843 4.582M12 3a8.997 8.997 0 00-7.843 4.582m15.686 0A11.953 11.953 0 0112 10.5c-2.998 0-5.74-1.1-7.843-2.918m15.686 0A8.959 8.959 0 0121 12c0 .778-.099 1.533-.284 2.253M3 12a8.959 8.959 0 00.284 2.253" />
            </svg>
            lukashev.studio
          </a>

          <span style={{ color: "#222" }}>·</span>

          <a href="https://www.instagram.com/lukashevmixing/" target="_blank" rel="noopener noreferrer"
            className="flex items-center gap-1.5 text-sm transition-colors"
            style={{ color: "#555" }}
            onMouseEnter={e => (e.currentTarget.style.color = "#999")}
            onMouseLeave={e => (e.currentTarget.style.color = "#555")}
          >
            <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <rect x="2" y="2" width="20" height="20" rx="5" ry="5" strokeWidth={1.5} />
              <circle cx="12" cy="12" r="4" strokeWidth={1.5} />
              <circle cx="17.5" cy="6.5" r="0.5" fill="currentColor" />
            </svg>
            @lukashevmixing
          </a>

          <span style={{ color: "#222" }}>·</span>

          <a href="https://youtu.be/I_F9cqSe-6A?si=ozcpHcoOjfQWlU2B" target="_blank" rel="noopener noreferrer"
            className="flex items-center gap-1.5 text-sm transition-colors"
            style={{ color: "#555" }}
            onMouseEnter={e => (e.currentTarget.style.color = "#999")}
            onMouseLeave={e => (e.currentTarget.style.color = "#555")}
          >
            <svg className="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 24 24">
              <path d="M23.498 6.186a3.016 3.016 0 00-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 00.502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 002.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 002.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z" />
            </svg>
            How to spot a fake
          </a>
        </div>

        <a href="https://www.paypal.com/pool/9fLEzVKQFP?sr=wccr" target="_blank" rel="noopener noreferrer"
          className="inline-flex items-center gap-2 text-xs px-4 py-2 rounded-full transition-colors mb-6"
          style={{ border: "1px solid #3a2e1a", color: "#C9A96E", backgroundColor: "#0d0a05" }}
          onMouseEnter={e => { e.currentTarget.style.backgroundColor = "#1a1408"; e.currentTarget.style.borderColor = "#C9A96E"; }}
          onMouseLeave={e => { e.currentTarget.style.backgroundColor = "#0d0a05"; e.currentTarget.style.borderColor = "#3a2e1a"; }}
        >
          <svg className="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 24 24">
            <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z" />
          </svg>
          Support this project
        </a>

        <p className="text-xs" style={{ color: "#2a2a2a" }}>
          For reference only — not a professional authentication service
        </p>
      </footer>
    </main>
  );
}
