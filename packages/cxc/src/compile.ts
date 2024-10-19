import htemplate from "./htemplate.txt";
import ctemplate from "./ctemplate.txt";

/**
 * Compiles a file and returns the compiled code
 * @param file
 * @param name the name of a file, e. g. "my_magic_component"
 * @returns
 */
export async function compile(file: string, name: string) {
  const cxFile = Bun.file(file);

  let cxContent = await cxFile.text();

  const mainFuctionName = `render_${name}`;
  const fileID = "template_header_" + mainFuctionName;
  const propsName = `${name
    .split("_")
    .map((s) => s[0].toUpperCase() + s.slice(1))
    .join("")}Props`;

  let propsStruct = "";
  if (cxContent.startsWith("({")) {
    const propsEndIndex = cxContent.indexOf("})");
    propsStruct = cxContent.slice(2, propsEndIndex);
    cxContent = cxContent.slice(propsEndIndex + 2);
  }

  let prependFile = "";
  const firstHtmlIndex = cxContent.indexOf("\n<");

  if (firstHtmlIndex > 0) {
    prependFile = cxContent.slice(0, firstHtmlIndex);
    cxContent = cxContent.slice(firstHtmlIndex);
  }

  if (prependFile.indexOf("#include <string.h>") === -1) {
    prependFile += "#include <string.h>\n";
  }

  if (prependFile.indexOf("#include <stdio.h>") === -1) {
    prependFile += "#include <stdio.h>\n";
  }

  if (prependFile.indexOf("#include <stdlib.h>") === -1) {
    prependFile += "#include <stdlib.h>\n";
  }

  let cxFuctionCode = "";
  let currentContext: "text" | "closure" | undefined = undefined;
  let lastClosureId = -1;
  let closureExitsToSkip = 0;
  let charsToSkip = 0;

  const openTextContext = () => {
    if (currentContext) {
      throw new Error(
        `Tried to open "text" context, while context "${currentContext}" was open`
      );
    }
    currentContext = "text";
    cxFuctionCode += `strcpy(output + strlen(output), "`;
  };

  const closeTextContext = () => {
    cxFuctionCode += `");\n`;
    currentContext = undefined;
  };

  const openClosureContext = () => {
    if (currentContext) {
      throw new Error(
        `Tried to open "closure" context, while context "${currentContext}" was open`
      );
    }
    lastClosureId++;
    currentContext = "closure";
    cxFuctionCode += `\n // Closure ${lastClosureId}\n`;
  };

  const closeClosureContext = () => {
    cxFuctionCode += `\n\n`;
    currentContext = undefined;
  };

  for (let index = 0; index < cxContent.length; index++) {
    if (charsToSkip > 0) {
      charsToSkip--;
      continue;
    }

    const char = cxContent[index];

    if (char === "}") {
      if (closureExitsToSkip > 0) {
        closureExitsToSkip--;
        continue;
      }

      if (currentContext === "closure") {
        closeClosureContext();

        continue;
      }
    }

    if (char === "{") {
      if (currentContext === "closure") {
        closureExitsToSkip++;
        continue;
      }
      closeTextContext();
      openClosureContext();
      if (cxContent[index + 1] === "=") {
        cxFuctionCode += `strcpy(output + strlen(output), `;

        let i = 0;
        while (true) {
          const varChar = cxContent[index + 2 + i];
          if (varChar === "}") {
            break;
          }
          cxFuctionCode += varChar;
          i++;
        }
        cxFuctionCode += ");\n";

        charsToSkip = i + 2;

        closeClosureContext();
      }
      continue;

      // TODO
      //   if (cxContent[index + 1] === "=") {
      //     cxFuctionCode += char;
      //     index++;
      //     continue;
      //   }
    }

    if (!currentContext) {
      openTextContext();
    }

    if (currentContext === "text") {
      if (char === '"') {
        cxFuctionCode += "\\";
      }
    }
    if (currentContext === "text" && char === "\n") {
      cxFuctionCode += '" \\\n"';
    } else {
      cxFuctionCode += char;
    }
  }
  if (currentContext === "closure") {
    closeClosureContext();
  } else if (currentContext === "text") {
    closeTextContext();
  } else {
    throw new Error("No context was open");
  }

  //console.log(cxFuctionCode);

  return [
    ctemplate
      .replace("%%PROPS_NAME%%", propsName)
      .replace("%%FUNC_NAME%%", mainFuctionName)
      .replace("%%CODE%%", cxFuctionCode)
      .replace("%%NAME%%", name)
      .replace("%%PREPEND%%", prependFile),
    htemplate
      .replaceAll("%%PROPS_NAME%%", propsName)
      .replace("%%PROPS%%", propsStruct)
      .replace("%%FUNC_NAME%%", mainFuctionName)
      .replaceAll("%%FILEID%%", fileID)
      .replace("%%PREPEND%%", prependFile),
  ];

  //   await Bun.write("./dist/template.c", content);
}
