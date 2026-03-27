import Anthropic from "@anthropic-ai/sdk";
import { NextRequest, NextResponse } from "next/server";
import fs from "fs";
import path from "path";

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
- TRANSFORMERLESS design — genuine TLM103 has NO transformer on the PCB by design. If a transformer is visible on the PCB, it is a fake.
- Screws securing the headgrille mesh to body are flush with the body on genuine units. Fakes typically have recessed screws due to poor manufacturing tolerances.
- Mic stand adapter/thread collar: genuine is METAL. Fake units use plastic adapters.
- Body label/sticker: genuine has a perfectly applied thin label — no bubbling, no lifting, flawless German craftsmanship. Fakes may show bubbling or uneven application under the label.
- Wooden box (if included): genuine Neumann packaging has DOVETAIL JOINTS on the wooden case and the Neumann logo is CARVED into the lid. Fakes have stickered logos and cheaply assembled boxes without dovetail joinery.
- Internal glue: genuine units have no visible glue globs. Fakes often have visible globs of adhesive on internal components, sometimes visible through the mesh grille.
- PCB markings: genuine TLM103 boards have Neumann printed/silkscreened on the circuit board. Fakes may or may not have markings.
- NOTE: Fakes are becoming increasingly sophisticated — when in doubt, recommend professional verification.

**PCB / Amplifier Board (Genuine)**:
- Genuine U87/TLM103 PCBs use high-quality FR4 board with clean silk-screening
- Components are through-hole and SMD of known brands; solder joints are clean and uniform
- Genuine U87 uses a discrete FET-based circuit; look for the characteristic BF862 or K170 FET
- No excess flux residue or sloppy hand-soldering on genuine units
- PCB traces should be clean with no bridging or cold joints
- Genuine boards have Neumann part numbers or date codes silkscreened on the board
- Capacitors and resistors should be properly rated (63V or higher for audio caps)
- Fake boards often use generic jellybean components, cheap electrolytic caps, and poor layout
- ABSENCE OF MARKINGS — CRITICAL: If a PCB has no silk-screen markings at all (no part numbers, no model codes, no date codes, no reference designators), this is a definitive indicator of a FAKE or DIY build. Genuine Neumann boards always have proper silk-screened labeling.

**PCB Fake Indicators — CRITICAL (expert-verified, from teardown comparison)**:
- FOUR BLUE TOROIDAL INDUCTORS (chokes) in the center of the PCB are a confirmed sign of a fake U87. Genuine U87 boards do not have this arrangement of blue toroidal components in the middle of the board.
- LARGE YELLOW TRANSFORMER on PCB: Fake U87Ai boards often contain a large, bulky yellow transformer/toroid component. The genuine U87Ai PCB uses a small, flat "CIRCUIT DIAGRAM" labeled transformer — compact and low-profile. A large yellow transformer is a strong red flag.
- NOTE: Silk-screen markings such as "U89" or "30U4" on a PCB are NOT a red flag — Neumann uses the same PCB platform across multiple models (U87, U89, etc.), so these markings appear on genuine boards as well. Do NOT flag this as suspicious.

**Body & Hardware Indicators — CRITICAL (expert-verified)**:
- SCREWS: Genuine U87Ai uses FLAT HEAD (slotted) screws. If Phillips/cross-head screws are visible on the body or connector area, this is a strong fake indicator.
- XLR CONNECTOR FINISH: Genuine U87Ai has a NICKEL-finish XLR connector — matte, warm gray tone. Fake connectors are CHROME — bright, shiny, mirror-like finish. Chrome XLR = almost certainly fake.
- CAPSULE MOUNT RING: Genuine U87Ai capsule ring is clean white/light gray plastic with a precise machined look. Fake capsule rings are YELLOWISH/cream colored plastic with a cheap injection-molded appearance and visible gold-colored contact pins around the ring. Yellow tint on the capsule mount ring = strong fake indicator.
- PLASTIC BODY COLOR: Genuine U87Ai body plastic is neutral gray with sharp, precise edges. Fake bodies have a yellowish tint and rounded, imprecise edges — typical of low-quality injection molding.
- VINTAGE U87 NOTE: The vintage/original U87 (pre-AI era) was never widely counterfeited. A key identifier of a genuine vintage U87 is the presence of a battery compartment on the body. If the mic has a battery compartment and appears vintage, it is very likely genuine.

**Capsule (Optional but valuable)**:
- Genuine Neumann K87 capsule (in U87): dual-diaphragm, gold-sputtered Mylar; very fine and even tensioning; brass backplate visible through diaphragm
- TLM103 uses K103 capsule: single large-diaphragm cardioid; similar quality construction
- Fake capsules often have uneven diaphragm tensioning, visible creases, or poor gold sputtering
- Genuine capsule mounting hardware is machined; fakes use yellowish plastic with visible gold contact pins

**Common Fake Indicators**:
- Blurry or poorly etched Neumann logo
- Incorrect badge shape or placement
- Uneven headgrille mesh or visible weld/seam marks
- Poor body finish (rough casting, visible seams, paint inconsistency)
- Incorrect switch labeling or placement
- Wrong proportions compared to genuine spec
- Chrome XLR connector (should be nickel)
- Phillips/cross-head screws (should be flat head)
- Large yellow transformer on PCB
- Yellowish capsule mount ring with gold contact pins
- Capsule not visible or incorrectly positioned
- Serial number appears printed/stickered rather than stamped
- Chinese replicas often have "NEUMANN" spelled incorrectly or logo style differences
- Fake PCBs: generic components, poor soldering, no Neumann markings, cheap capacitors

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

// Reference images with captions — loaded once at module level
const REFERENCES: { file: string; mediaType: "image/jpeg" | "image/png"; caption: string }[] = [
  {
    file: "pcb-overview.jpg",
    mediaType: "image/jpeg",
    caption: "REFERENCE 1 — PCB overview side by side. LEFT = FAKE U87 PCB. RIGHT = GENUINE U87 PCB. Note the overall component layout and quality differences.",
  },
  {
    file: "pcb-transformer-detail.jpg",
    mediaType: "image/jpeg",
    caption: "REFERENCE 2 — Transformer close-up. LEFT = GENUINE: small flat transformer labeled 'CIRCUIT DIAGRAM' (circled). RIGHT = FAKE: large bulky YELLOW TRANSFORMER (circled). A large yellow transformer is a definitive fake indicator.",
  },
  {
    file: "genuine-flat-screws.jpg",
    mediaType: "image/jpeg",
    caption: "REFERENCE 3 — Screws. GENUINE U87 uses FLAT HEAD (slotted) screws visible on the connector area. Phillips/cross-head screws = fake.",
  },
  {
    file: "capsule-ring.png",
    mediaType: "image/png",
    caption: "REFERENCE 4 — Capsule mount ring viewed from above. LEFT = FAKE: yellowish/cream plastic with visible gold contact pins around the ring. RIGHT = GENUINE: clean white/gray plastic, precise machined appearance, no visible pins.",
  },
  {
    file: "xlr-connector.png",
    mediaType: "image/png",
    caption: "REFERENCE 5 — XLR connector finish. LEFT = FAKE: bright chrome mirror finish. RIGHT = GENUINE: matte nickel finish, warmer gray tone. Chrome = fake, Nickel = genuine.",
  },
];

function loadReferenceImages() {
  const refDir = path.join(process.cwd(), "public", "references");
  return REFERENCES.map((ref) => {
    const filePath = path.join(refDir, ref.file);
    const data = fs.readFileSync(filePath).toString("base64");
    return { ...ref, data };
  });
}

export async function POST(req: NextRequest) {
  try {
    const body = await req.json();
    const { images } = body as { images: { data: string; mediaType: string }[] };

    if (!images || images.length === 0) {
      return NextResponse.json({ error: "No images provided" }, { status: 400 });
    }

    // Load reference images
    const refs = loadReferenceImages();

    // Build reference block: caption + image interleaved
    const referenceContent: Anthropic.ContentBlockParam[] = [
      {
        type: "text",
        text: "The following are EXPERT REFERENCE IMAGES from a professional teardown of genuine vs fake Neumann U87Ai microphones. Treat these as visual ground truth when analyzing the user's photos below.",
      },
      ...refs.flatMap((ref) => [
        { type: "text" as const, text: ref.caption },
        {
          type: "image" as const,
          source: { type: "base64" as const, media_type: ref.mediaType, data: ref.data },
        },
      ]),
      {
        type: "text",
        text: `— END OF REFERENCE IMAGES —\n\nNow analyze the following ${images.length > 1 ? `${images.length} user-submitted images` : "user-submitted image"} and determine authenticity. Respond with the JSON object as specified.`,
      },
    ];

    // User's uploaded images
    const userContent: Anthropic.ContentBlockParam[] = images.map((img) => ({
      type: "image" as const,
      source: {
        type: "base64" as const,
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
          content: [...referenceContent, ...userContent],
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
