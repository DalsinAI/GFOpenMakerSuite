/** One layer of a mesh: the triangle edges that cross a horizontal plane.
 * Walls only. There is no infill and no G-code. */

export function contoursAt(tris: Float32Array, z: number): { segments: Float32Array; length: number } {
  const out: number[] = [];
  let length = 0;
  for (let i = 0; i < tris.length; i += 9) {
    const hit: number[] = [];
    for (let e = 0; e < 3; e++) {
      const a = i + e * 3;
      const b = i + ((e + 1) % 3) * 3;
      const z0 = tris[a + 2]!;
      const z1 = tris[b + 2]!;
      const d = z1 - z0;
      if (Math.abs(d) < 1e-8) continue;
      const t = (z - z0) / d;
      if (t < -1e-4 || t > 1 + 1e-4) continue;
      const tt = t < 0 ? 0 : t > 1 ? 1 : t;
      hit.push(tris[a]! + (tris[b]! - tris[a]!) * tt, tris[a + 1]! + (tris[b + 1]! - tris[a + 1]!) * tt);
    }
    if (hit.length < 4) continue;
    out.push(hit[0]!, hit[1]!, z + 0.05, hit[2]!, hit[3]!, z + 0.05);
    length += Math.hypot(hit[2]! - hit[0]!, hit[3]! - hit[1]!);
  }
  return { segments: Float32Array.from(out), length };
}
