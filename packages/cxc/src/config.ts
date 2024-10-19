import parser from "command-line-args";
const optionDefinitions = [
  { name: "src", type: String, defaultOption: true },
  { name: "outDir", type: String, defaultValue: "./dist" },
  { name: "debug", type: String, defaultValue: false },
];

const parsed = parser(optionDefinitions);

const configFile = await Bun.file("./cx.json").json();

export const config = {
  outDir: configFile.outDir || parsed.outDir,
  src: configFile.src || parsed.src,
  debug: configFile.debug || parsed.debug,
};
