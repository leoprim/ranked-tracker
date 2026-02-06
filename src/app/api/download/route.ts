import { NextResponse } from 'next/server';

const GITHUB_REPO = 'leoprim/ranked-tracker';

export async function GET() {
  try {
    // Fetch the latest release tagged with obs-plugin-v*
    const res = await fetch(
      `https://api.github.com/repos/${GITHUB_REPO}/releases?per_page=10`,
      {
        headers: {
          Accept: 'application/vnd.github.v3+json',
          // Optional: add a token for higher rate limits
          // Authorization: `Bearer ${process.env.GITHUB_TOKEN}`,
        },
        next: { revalidate: 300 }, // Cache for 5 minutes
      },
    );

    if (!res.ok) {
      return NextResponse.json(
        { error: 'Failed to fetch releases' },
        { status: 502 },
      );
    }

    const releases = await res.json();

    // Find the latest obs-plugin release with an .exe asset
    for (const release of releases) {
      if (!release.tag_name?.startsWith('obs-plugin-v')) {
        continue;
      }

      const exeAsset = release.assets?.find(
        (asset: { name: string }) => asset.name.endsWith('.exe'),
      );

      if (exeAsset) {
        // Redirect to the direct download URL
        return NextResponse.redirect(exeAsset.browser_download_url, 302);
      }
    }

    return NextResponse.json(
      { error: 'No installer found. Check back after the first release.' },
      { status: 404 },
    );
  } catch {
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 },
    );
  }
}
