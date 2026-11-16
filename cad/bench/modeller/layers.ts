/** One layer of a mesh: the triangle edges that cross a horizontal plane.
 * Walls only. There is no infill and no G-code. */

export function contoursAt(tris: Float32Array, z: number): { segments: Float32Array; length: number } {
  const out: number[] = [];
  let length = 0;
  for (let i = 0; i < tris.length; i += 9) {
    const hit: number[] = [];
    // An edge is cut when its ends are on two sides. A point on the plane
    // counts as above, so a triangle gives none or two points, and an edge
    // lying in the plane is wall once, not once for each triangle beside it.
    for (let e = 0; e < 3; e++) {
      const a = i + e * 3;
      const b = i + ((e + 1) % 3) * 3;
      const z0 = tris[a + 2]!;
      const z1 = tris[b + 2]!;
      if (z0 < z === z1 < z) continue;
      const t = (z - z0) / (z1 - z0);
      hit.push(tris[a]! + (tris[b]! - tris[a]!) * t, tris[a + 1]! + (tris[b + 1]! - tris[a + 1]!) * t);
    }
    if (hit.length < 4) continue;
    out.push(hit[0]!, hit[1]!, z + 0.05, hit[2]!, hit[3]!, z + 0.05);
    length += Math.hypot(hit[2]! - hit[0]!, hit[3]! - hit[1]!);
  }
  return { segments: Float32Array.from(out), length };
}
