import { NextRequest, NextResponse } from "next/server";

export async function POST(req: NextRequest) {
  try {
    const { data, mediaType } = await req.json();

    const buffer = Buffer.from(data, "base64");
    const blob = new Blob([buffer], { type: mediaType });

    const formData = new FormData();
    formData.append("encoded_image", blob, "image.jpg");
    formData.append("image_content", "");
    formData.append("filename", "");
    formData.append("hl", "en");

    const response = await fetch(
      "https://www.google.com/searchbyimage/upload",
      {
        method: "POST",
        body: formData,
        redirect: "follow",
        headers: {
          "User-Agent":
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
          Referer: "https://images.google.com/",
        },
      }
    );

    return NextResponse.json({ url: response.url });
  } catch (err) {
    console.error("Reverse search error:", err);
    return NextResponse.json({ error: "Search failed" }, { status: 500 });
  }
}
