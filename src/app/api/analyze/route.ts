import Anthropic from "@anthropic-ai/sdk";
import { NextRequest, NextResponse } from "next/server";

const client = new Anthropic({ apiKey: process.env.ANTHROPIC_API_KEY });

const SYSTEM_PROMPT = `You are an expert microphone authenticator specializing in Neumann microphones, particularly the U87 and TLM103 models. You have deep knowledge of genuine Neumann products and how to spot counterfeits.

Key authenticity markers you examine:

**Neumann U87 (Genuine)**:
- Headgrille: Fine, evenly-spaced mesh; no visible gaps or irregularities; slightly convex dome shape
- Badge: High-quality etched or stamped Neumann logo; consistent lettering; proper placement on upper body
- Body finish: Smooth matte or nickel/chrome finish; no rough patches or casting defects
- Switches: Pad and low-cut switches should have clean, crisp edges; proper labeling (-10dB pad, 80Hz low-cut)
- Serial number: Stamped on back; typically 5-6 digits; should look engraved not printed
- Capsule (visible through grille): Should appear centered with even suspension
- Connector: XLR connector should be flush and properly aligned

**Neumann TLM103 (Genuine)**:
- Single large-diaphragm cardioid only — no polar pattern switch
- Headgrille: Slightly different mesh pattern than U87; characteristic bulge profile
- No pad switch on body (pad is in shock mount only)
- Badge placement: On front of body
- Serial number: Stamped on rear
- Body: Clean, smooth finish; proper proportions

**PCB / Amplifier Board (Genuine)**:
- Genuine U87/TLM103 PCBs use high-quality FR4 board with clean silk-screening
- Components are through-hole and SMD of known brands; solder joints are clean and uniform
- Genuine U87 uses a discrete FET-based circuit; look for the characteristic BF862 or K170 FET
- No excess flux residue or sloppy hand-soldering on genuine units
- PCB traces should be clean with no bridging or cold joints
- Genuine boards have Neumann part numbers or date codes silkscreened on the board
- Capacitors and resistors should be properly rated (63V or higher for audio caps)
- Fake boards often use generic jellybean components, cheap electrolytic caps, and poor layout

**PCB Fake Indicator — CRITICAL (expert-verified)**:
- FOUR BLUE TOROIDAL INDUCTORS (chokes) in the center of the PCB are a confirmed sign of a fake U87. Genuine U87 boards do not have this arrangement of blue toroidal components in the middle of the board.
- NOTE: Silk-screen markings such as "U89" or "30U4" on a PCB are NOT a red flag — Neumann uses the same PCB platform across multiple models (U87, U89, etc.), so these markings appear on genuine boards as well. Do NOT flag this as suspicious.
- Genuine U87 plastic body parts: edges are sharp and precise, color is neutral gray. Fake body parts: plastic appears yellowish, edges are rounded and imprecise, surface has a low-quality injection-molded look.
- VINTAGE U87 NOTE: The vintage/original U87 (pre-AI era) was never widely counterfeited. A key identifier of a genuine vintage U87 is the presence of a battery compartment on the body. If the mic has a battery compartment and appears vintage, it is very likely genuine.

**Capsule (Optional but valuable)**:
- Genuine Neumann K87 capsule (in U87): dual-diaphragm, gold-sputtered Mylar; very fine and even tensioning; brass backplate visible through diaphragm
- TLM103 uses K103 capsule: single large-diaphragm cardioid; similar quality construction
- Fake capsules often have uneven diaphragm tensioning, visible creases, or poor gold sputtering
- Genuine capsule mounting hardware is machined brass; fakes often use pot metal or plastic

**Common Fake Indicators**:
- Blurry or poorly etched Neumann logo
- Incorrect badge shape or placement
- Uneven headgrille mesh or visible weld/seam marks
- Poor body finish (rough casting, visible seams, paint inconsistency)
- Incorrect switch labeling or placement
- Wrong proportions compared to genuine spec
- Low-quality XLR connector
- Capsule not visible or incorrectly positioned
- Serial number appears printed/stickered rather than stamped
- Chinese replicas often have "NEUMANN" spelled incorrectly or logo style differences
- Clone brands sometimes leave their own model numbers visible
- Fake PCBs: generic components, poor soldering, no Neumann markings, cheap capacitors
- Fake capsules: uneven diaphragm, creases, poor gold coating, plastic mounting hardware

Always be specific about what you can and cannot determine from the provided images.

You must respond ONLY with a valid JSON object in this exact format:
{
  "identified_model": "Neumann U87" | "Neumann TLM103" | "Other microphone" | "Cannot determine from image",
  "authenticity_verdict": "Likely Genuine" | "Likely Fake" | "Suspicious — needs closer inspection" | "Inconclusive",
  "confidence_percentage": <integer 0-100>,
  "summary": "<2-3 sentence plain-English summary>",
  "positive_indicators": ["<indicator>", ...],
  "red_flags": ["<flag>", ...],
  "inconclusive_aspects": ["<aspect>", ...],
  "recommendations": ["<recommendation>", ...]
}

Do not include any text outside the JSON object.`;

export interface AnalysisResult {
  identified_model: string;
  authenticity_verdict: string;
  confidence_percentage: number;
  summary: string;
  positive_indicators: string[];
  red_flags: string[];
  inconclusive_aspects: string[];
  recommendations: string[];
}

export async function POST(req: NextRequest) {
  try {
    const body = await req.json();
    const { images } = body as { images: { data: string; mediaType: string }[] };

    if (!images || images.length === 0) {
      return NextResponse.json({ error: "No images provided" }, { status: 400 });
    }

    const imageContent: Anthropic.ImageBlockParam[] = images.map((img) => ({
      type: "image",
      source: {
        type: "base64",
        media_type: img.mediaType as "image/jpeg" | "image/png" | "image/gif" | "image/webp",
        data: img.data,
      },
    }));

    const response = await client.messages.create({
      model: "claude-opus-4-6",
      max_tokens: 4096,
      system: SYSTEM_PROMPT,
      messages: [
        {
          role: "user",
          content: [
            ...imageContent,
            {
              type: "text",
              text: `Please analyze ${images.length > 1 ? "these images" : "this image"} of a microphone and determine if it is a genuine Neumann U87 or TLM103, or a fake/clone. Respond with the JSON object as specified.`,
            },
          ],
        },
      ],
    });

    const textBlock = response.content.find((b) => b.type === "text");
    if (!textBlock || textBlock.type !== "text") {
      return NextResponse.json({ error: "No analysis returned" }, { status: 500 });
    }

    // Extract JSON (handle possible markdown code fences)
    let jsonText = textBlock.text.trim();
    const fenceMatch = jsonText.match(/```(?:json)?\s*([\s\S]*?)```/);
    if (fenceMatch) jsonText = fenceMatch[1].trim();

    const result: AnalysisResult = JSON.parse(jsonText);
    return NextResponse.json(result);
  } catch (err) {
    console.error("Analysis error:", err);
    const message = err instanceof Error ? err.message : "Analysis failed";
    return NextResponse.json({ error: message }, { status: 500 });
  }
}
