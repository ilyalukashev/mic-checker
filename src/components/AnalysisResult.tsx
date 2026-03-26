"use client";

import type { AnalysisData } from "@/app/page";

interface Props {
  data: AnalysisData;
}

function VerdictBadge({ verdict }: { verdict: string }) {
  const map: Record<string, { bg: string; text: string; border: string; icon: string }> = {
    "Likely Genuine":                        { bg: "#0a1a0a", text: "#6aaa6a", border: "#1a3a1a", icon: "✓" },
    "Likely Fake":                           { bg: "#1a0a0a", text: "#cc6666", border: "#3a1a1a", icon: "✗" },
    "Suspicious — needs closer inspection":  { bg: "#1a1500", text: "#C9A96E", border: "#3a3000", icon: "!" },
    Inconclusive:                            { bg: "#111",    text: "#666",    border: "#222",    icon: "?" },
  };
  const s = map[verdict] ?? map["Inconclusive"];
  return (
    <span
      className="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-md text-xs font-mono font-semibold"
      style={{ backgroundColor: s.bg, color: s.text, border: `1px solid ${s.border}` }}
    >
      <span>{s.icon}</span>
      {verdict}
    </span>
  );
}

function ConfidenceBar({ value }: { value: number }) {
  const color = value >= 75 ? "#6aaa6a" : value >= 50 ? "#C9A96E" : "#cc6666";
  return (
    <div className="mt-4">
      <div className="flex justify-between text-xs mb-1.5" style={{ color: "#555" }}>
        <span>Confidence</span>
        <span className="font-mono" style={{ color: "#888" }}>{value}%</span>
      </div>
      <div className="h-px overflow-hidden" style={{ backgroundColor: "#1a1a1a" }}>
        <div className="h-full transition-all" style={{ width: `${value}%`, backgroundColor: color }} />
      </div>
    </div>
  );
}

function Section({ title, items, type }: { title: string; items: string[]; type: "positive" | "negative" | "neutral" }) {
  if (items.length === 0) return null;
  const styles = {
    positive: { dot: "#4a8a4a", text: "#aaa",  title: "#6aaa6a" },
    negative: { dot: "#8a4a4a", text: "#aaa",  title: "#cc6666" },
    neutral:  { dot: "#333",    text: "#555",  title: "#444"    },
  };
  const s = styles[type];
  return (
    <div>
      <p className="text-xs uppercase tracking-widest mb-3 font-mono" style={{ color: s.title }}>{title}</p>
      <ul className="space-y-2">
        {items.map((item, i) => (
          <li key={i} className="flex items-start gap-3 text-sm">
            <span className="mt-2 w-1 h-1 rounded-full flex-shrink-0" style={{ backgroundColor: s.dot }} />
            <span style={{ color: s.text }}>{item}</span>
          </li>
        ))}
      </ul>
    </div>
  );
}

export default function AnalysisResult({ data }: Props) {
  return (
    <div style={{ gap: "1px", display: "flex", flexDirection: "column" }}>
      {/* Verdict */}
      <div className="rounded-t-xl p-5" style={{ backgroundColor: "#0d0d0d", border: "1px solid #1a1a1a" }}>
        <div className="flex items-start justify-between gap-4 flex-wrap">
          <div>
            <p className="text-xs font-mono uppercase tracking-widest mb-1" style={{ color: "#444" }}>Model</p>
            <p className="text-lg font-semibold text-white">{data.identified_model}</p>
          </div>
          <VerdictBadge verdict={data.authenticity_verdict} />
        </div>
        <ConfidenceBar value={data.confidence_percentage} />
        <p className="mt-4 text-sm leading-relaxed" style={{ color: "#888" }}>{data.summary}</p>
      </div>

      {/* Details */}
      <div className="p-5 space-y-6" style={{ backgroundColor: "#0d0d0d", border: "1px solid #1a1a1a", borderTop: "none" }}>
        <Section title="Positive indicators" items={data.positive_indicators} type="positive" />
        <Section title="Red flags"           items={data.red_flags}           type="negative" />
        <Section title="Could not verify"    items={data.inconclusive_aspects} type="neutral" />
      </div>

      {/* Recommendations */}
      {data.recommendations.length > 0 && (
        <div className="rounded-b-xl p-5" style={{ backgroundColor: "#0d0d0d", border: "1px solid #1a1a1a", borderTop: "none" }}>
          <p className="text-xs uppercase tracking-widest font-mono mb-3" style={{ color: "#5a4a2a" }}>Recommendations</p>
          <ul className="space-y-2">
            {data.recommendations.map((rec, i) => (
              <li key={i} className="flex items-start gap-3 text-sm">
                <span className="mt-2 w-1 h-1 rounded-full flex-shrink-0" style={{ backgroundColor: "#8a6a30" }} />
                <span style={{ color: "#888" }}>{rec}</span>
              </li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
}
