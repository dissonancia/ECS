#include "VMtranslator.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.vm>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];

    return EXIT_SUCCESS;
}

/*
String_Builder sb = {0};
if (!read_entire_file("meuarquivo.txt", &sb)) {
    fprintf(stderr, "Falha ao ler o arquivo\n");
    return 1;
}

// Criar uma String_View para percorrer o conteúdo
String_View content = sb_to_sv(sb);

while (content.count > 0) {
    // Extrai a próxima linha
    String_View line = sv_chop_by_delim(&content, '\n');
    
    // Remove espaços em branco no início e fim da linha
    line = sv_trim(line);

    // Se quiser ignorar linhas vazias ou comentários
    if (line.count == 0) continue;
    line = sv_strip_comment(line);

    // Processar a linha
    sv_println(line);
}

// Liberar memória
sb_free(sb);
*/
