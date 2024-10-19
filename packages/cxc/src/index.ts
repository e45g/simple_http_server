import { config } from "./config";
import { getFiles } from "./utils";
import { compile } from "./compile";
import { mkdir, readdir } from "node:fs/promises";
import { join } from "node:path";

if (config.debug) console.log("CX config:", config);

const files = await readdir(config.src, {
  withFileTypes: true,
  recursive: true,
});

if (!files.length) {
  console.log("No files found");
  process.exit(1);
}

const srcLength = join(config.src).split("/").length;

try {
  await mkdir(config.outDir, { recursive: true });
  for (const file of files) {
    if (file.isDirectory()) {
      continue;
    }

    const start = Bun.nanoseconds();

    const name = [
      file.parentPath.split("/").slice(srcLength),
      file.name.split(".")[0],
    ]
      .flat()
      .join("_");

    const [c, h] = await compile(join(file.parentPath, file.name), name);

    await Bun.write(join(config.outDir, name + ".c"), c);
    await Bun.write(join(config.outDir, name + ".h"), h);

    const end = Bun.nanoseconds();

    console.log(
      `✅ ${join(file.parentPath, file.name)} compiled to ${name} in ${
        (end - start) / 1000000
      } ms`
    );
  }
} catch (err) {
  console.error("❌ Error:", err);
  process.exit(1);
}

console.log("👍 Done!");
