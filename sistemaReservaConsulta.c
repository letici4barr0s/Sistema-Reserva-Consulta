#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PACIENTES 100
#define MAX_USUARIOS 50
#define MAX_NOME_USUARIO 50
#define MAX_SENHA_USUARIO 50
#define MAX_CPF 15
#define MAX_CELULAR 15

typedef struct {
    int id;
    char nome[100];
    struct tm dataNascimento;
    char cpf[MAX_CPF];
    char celular[MAX_CELULAR];
} Paciente;

typedef struct {
    char nomeUsuario[MAX_NOME_USUARIO];
    char senha[MAX_SENHA_USUARIO];
} Usuario;

typedef struct {
    int idConsulta;
    int idPaciente;
    char nomePaciente[100];
    int horaH;
    int horaM;
    char medico[50];
} Consulta;

typedef struct DesfazCancelamento{
    Consulta c[MAX_PACIENTES];
    int topo;
}Desfaz;

typedef struct FilaConsulta{
    Consulta consulta[MAX_PACIENTES];
    int inicio, fim;
}FilaConsulta;

Paciente *pilhaPacientes[MAX_PACIENTES];
int topoPilha = -1;


int totalConsulta = 0;

Consulta *consultas[MAX_PACIENTES];  
int totalPacientes = 0;
Paciente *pacientes[MAX_PACIENTES];
int totalUsuarios = 0;
Usuario *usuarios[MAX_USUARIOS];

Desfaz pilha;

void iniciarFila(FilaConsulta *fila){
    fila->inicio=0;
    fila->fim=0;
}

int filaVazia(FilaConsulta *f) {
    return f->inicio == f->fim;
}

int filaCheia(FilaConsulta *f) {
    return (f->fim + 1) % MAX_PACIENTES == f->inicio;
}

void filaEspera(FilaConsulta *fila, Consulta c) {
    if (filaCheia(fila)) {
        printf("ERRO!! Fila de espera atingiu sua quantidade máxima.\n");
        return;
    }
    fila->consulta[fila->fim] = c;

    fila->fim = (fila->fim + 1) % MAX_PACIENTES;
}

void carregarPacientesDoArquivo() {
    FILE *arquivo = fopen("pacientes.txt", "r");
    if (arquivo != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), arquivo)) {
            Paciente *p = malloc(sizeof(Paciente));
            int d, m, a;

            sscanf(linha, "%d|%99[^|]|%d/%d/%d|%14[^|]|%14[^\n]",
                   &p->id, p->nome, &d, &m, &a, p->cpf, p->celular);

            p->dataNascimento.tm_mday = d;
            p->dataNascimento.tm_mon = m - 1;
            p->dataNascimento.tm_year = a - 1900;

            pacientes[totalPacientes++] = p;
        }
        fclose(arquivo);
    }
}

void carregarConsultasDoArquivo() {
    FILE *arquivo = fopen("dadosConsulta.txt", "r");
    if (arquivo != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), arquivo)) {
            Consulta *c = malloc(sizeof(Consulta));
            if (sscanf(linha, "Nome: %49[^|]|ID consulta: %d|Horario: %d:%d|Medico: %[^\n]",
                       c->nomePaciente, &c->idConsulta, &c->horaH, &c->horaM, c->medico) == 5) {
                consultas[totalConsulta++] = c;
            }
        }
        fclose(arquivo);
    } else {
        printf("Nenhuma consulta encontrada para carregar.\n");
    }
}

Consulta atenderPaciente(FilaConsulta *fila) {
    Consulta c = {0};

    if (filaVazia(fila)) {
        printf("ERRO! Fila vazia.\n");
        return c;
    }

    c = fila->consulta[fila->inicio]; 
    fila->inicio = (fila->inicio + 1) % MAX_PACIENTES;

    return c;
}

void pushPaciente(Paciente *p) {
    if (topoPilha < MAX_PACIENTES - 1) {
        pilhaPacientes[++topoPilha] = p;
    }
}

Paciente* popPaciente() {
    if (topoPilha >= 0) {
        return pilhaPacientes[topoPilha--];
    }
    return NULL;
}

void limparTela(){
    system("cls");
}

void iniciarPilha(Desfaz *pilha){
    pilha->topo = -1;
}

void empilharConsulta(Desfaz *d, Consulta c) {
    if (d->topo < MAX_PACIENTES - 1) {
        d->topo++;
        d->c[d->topo] = c;
    } else {
        printf("A pilha está cheia.\n");
    }
}

Consulta desempilharConsulta(Desfaz *pilha, int *sucesso) {
    Consulta consultaVazia={0};

    if (pilha->topo >= 0) {
        *sucesso = 1;
        return pilha->c[pilha->topo--];
        
    } else {
            printf("Nenhuma consulta para restaurar.\n");
            *sucesso=0;
            return consultaVazia;   
      }
}

void restaurarConsulta(Desfaz *pilha) {
    int sucesso;
    Consulta c = desempilharConsulta(pilha, &sucesso);
    if (sucesso) {
        *consultas = realloc(consultas, (totalConsulta + 1) * sizeof(Consulta));
        if (consultas == NULL) {
            printf("Erro ao alocar memoria para restaurar consulta.\n");
            return;
        }
        *consultas[totalConsulta++] = c;
        
        FILE *arquivo = fopen("dadosConsulta.txt", "a");
        if (arquivo != NULL) {
            fprintf(arquivo, "Nome: %s|ID consulta: %d|Horario: %02d:%02d|Medico: %s\n",
                    c.nomePaciente, c.idConsulta, c.horaH, c.horaM, c.medico);
            fclose(arquivo);
            printf("Consulta reativada com sucesso.\n");
        } else {
            printf("Erro ao reabrir arquivo de consultas.\n");
        }
    }
}

void adicionarPaciente() {
    Paciente *p = (Paciente*)malloc(sizeof(Paciente));
    p->id = totalPacientes + 1;

    printf("Digite o nome do paciente: ");
    getchar(); 
    fgets(p->nome, sizeof(p->nome), stdin);
    p->nome[strcspn(p->nome, "\n")] = '\0';  

    printf("Digite a data de nascimento do paciente (DD MM AAAA): ");
    scanf("%d %d %d", &p->dataNascimento.tm_mday, &p->dataNascimento.tm_mon, &p->dataNascimento.tm_year);

    p->dataNascimento.tm_mon--;  
    p->dataNascimento.tm_year -= 1900;  

    printf("Digite o CPF do paciente: ");
    getchar();  
    fgets(p->cpf, sizeof(p->cpf), stdin);
    p->cpf[strcspn(p->cpf, "\n")] = '\0';

    printf("Digite o número de celular do paciente: ");
    fgets(p->celular, sizeof(p->celular), stdin);
    p->celular[strcspn(p->celular, "\n")] = '\0';

    pacientes[totalPacientes++] = p;
    pushPaciente(p); 

    FILE *arquivo = fopen("pacientes.txt", "a");
    if (arquivo != NULL) {
        fprintf(arquivo, "%d|%s|%02d/%02d/%04d|%s|%s\n",
                p->id, p->nome,
                p->dataNascimento.tm_mday,
                p->dataNascimento.tm_mon + 1,  
                p->dataNascimento.tm_year + 1900,  
                p->cpf, p->celular);
        fclose(arquivo);
    }
    limparTela();
    printf("Paciente adicionado com sucesso!\n");
    
    char opcao;
    printf("Deseja adicionar outro paciente? (S/N): ");
    scanf(" %c", &opcao);
    if (opcao == 'S' || opcao == 's') {
        adicionarPaciente();
    }
}

void listarPacientes() {
    printf("\nLista de Pacientes:\n");
    FILE *arquivo = fopen("pacientes.txt", "r");
    if (arquivo != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), arquivo)) {
            int id;
            char nome[100];
            int d, m, a;
            char cpf[MAX_CPF], celular[MAX_CELULAR];

            sscanf(linha, "%d|%99[^|]|%d/%d/%d|%14[^|]|%14[^\n]",
                   &id, nome, &d, &m, &a, cpf, celular);
            printf("ID: %d | Nome: %s | Data de Nascimento: %02d/%02d/%04d | CPF: %s | Celular: %s\n",
                   id, nome, d, m, a, cpf, celular);
        }
        fclose(arquivo);
    } else {
        printf("Nenhum paciente encontrado.\n");
    }
}

void removerPaciente() {
    int id;
    printf("Digite o ID do paciente a ser removido: ");
    scanf("%d", &id);

    FILE *arquivo = fopen("pacientes.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    Paciente *removido = NULL;

    if (arquivo != NULL && temp != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), arquivo)) {
            int pacienteId;
            sscanf(linha, "%d", &pacienteId);
            if (pacienteId != id) {
                fputs(linha, temp);
            }
        }
        limparTela();
        fclose(arquivo);
        fclose(temp);

        remove("pacientes.txt");
        rename("temp.txt", "pacientes.txt");

        if (removido != NULL) {
            pushPaciente(removido); 
        }

        limparTela();
        printf("Paciente removido com sucesso!\n");
    } else {
        printf("Erro ao acessar os arquivos.\n");
    }   
}

void editarPaciente() {
    int id;
    printf("Digite o ID do paciente a ser editado: ");
    scanf("%d", &id);

    FILE *arquivo = fopen("pacientes.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (arquivo != NULL && temp != NULL) {
        char linha[256];
        int encontrado = 0;
        while (fgets(linha, sizeof(linha), arquivo)) {
            int pacienteId;
            char nome[100], cpf[MAX_CPF], celular[MAX_CELULAR];
            int d, m, a;

            sscanf(linha, "%d|%99[^|]|%d/%d/%d|%14[^|]|%14[^\n]",
                   &pacienteId, nome, &d, &m, &a, cpf, celular);

            if (pacienteId == id) {
                encontrado = 1;
                printf("Digite o novo nome para o paciente (atual: %s): ", nome);
                getchar(); 
                fgets(nome, sizeof(nome), stdin);
                nome[strcspn(nome, "\n")] = '\0';

                printf("Digite a nova data de nascimento para o paciente (atual: %02d/%02d/%04d): ", d, m, a);
                scanf("%d %d %d", &d, &m, &a);

                printf("Digite o novo CPF (atual: %s): ", cpf);
                getchar();
                fgets(cpf, sizeof(cpf), stdin);
                cpf[strcspn(cpf, "\n")] = '\0';

                printf("Digite o novo celular (atual: %s): ", celular);
                fgets(celular, sizeof(celular), stdin);
                celular[strcspn(celular, "\n")] = '\0';

                fprintf(temp, "%d|%s|%02d/%02d/%04d|%s|%s\n",
                        pacienteId, nome, d, m, a, cpf, celular);
            } else {
                fputs(linha, temp);
            }
        }
        limparTela();
        fclose(arquivo);
        fclose(temp);

        remove("pacientes.txt");
        rename("temp.txt", "pacientes.txt");

        if (encontrado) {
            printf("Paciente editado com sucesso!\n");
        } else {
            printf("Paciente não encontrado.\n");
        }
    } else {
        printf("Erro ao acessar os arquivos.\n");
    }
    
}

void registrarUsuario() {
    Usuario *novoUsuario = (Usuario*)malloc(sizeof(Usuario));
    printf("Digite o nome de usuário: ");
    getchar();  
    fgets(novoUsuario->nomeUsuario, sizeof(novoUsuario->nomeUsuario), stdin);
    novoUsuario->nomeUsuario[strcspn(novoUsuario->nomeUsuario, "\n")] = '\0'; 

    printf("Digite a senha: ");
    fgets(novoUsuario->senha, sizeof(novoUsuario->senha), stdin);
    novoUsuario->senha[strcspn(novoUsuario->senha, "\n")] = '\0'; 

    usuarios[totalUsuarios++] = novoUsuario;

    FILE *arquivo = fopen("usuarios.txt", "a");
    if (arquivo != NULL) {
        fprintf(arquivo, "%s|%s\n", novoUsuario->nomeUsuario, novoUsuario->senha);
        fclose(arquivo);
    }

    printf("Usuário registrado com sucesso!\n");

    char opcao;
    printf("Deseja registrar outro usuário? (S/N): ");
    scanf(" %c", &opcao);
    if (opcao == 'S' || opcao == 's') {
        registrarUsuario();
    }
}

int autenticarUsuario() {
    char nomeUsuario[MAX_NOME_USUARIO];
    char senha[MAX_SENHA_USUARIO];

    printf("Digite o nome de usuário: ");
    getchar();  
    fgets(nomeUsuario, sizeof(nomeUsuario), stdin);
    nomeUsuario[strcspn(nomeUsuario, "\n")] = '\0';

    printf("Digite a senha: ");
    fgets(senha, sizeof(senha), stdin);
    senha[strcspn(senha, "\n")] = '\0';

    FILE *arquivo = fopen("usuarios.txt", "r");
    if (arquivo != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), arquivo)) {
            char usuario[MAX_NOME_USUARIO], pass[MAX_SENHA_USUARIO];
            sscanf(linha, "%49[^|]|%49[^\n]", usuario, pass);
            if (strcmp(usuario, nomeUsuario) == 0 && strcmp(pass, senha) == 0) {
                fclose(arquivo);
                return 1;
            }
        }
        fclose(arquivo);
    }
    limparTela();
    printf("Usuário ou senha inválidos.\n");
    
    return 0; 
}

Paciente *buscaPaciente(int id, int posicao) {
    if (posicao >= totalPacientes) {
        return NULL;
    }
    if (pacientes[posicao]->id == id) {
        return pacientes[posicao];
    }
    return buscaPaciente(id, posicao + 1);
}


void arquivo(int tipo, Consulta c) {
    if (tipo == 1) {
        FILE *adcConsul = fopen("dadosConsulta.txt", "a");
        if (adcConsul != NULL) {
            fprintf(adcConsul,"Nome: %s|ID consulta: %d|Horario: %02d:%02d|Medico: %s\n",c.nomePaciente, c.idConsulta, c.horaH, c.horaM, c.medico);
            fclose(adcConsul);
            printf("Consulta armazenada com sucesso.\n");
        } else {
            printf("Erro ao abrir arquivo.\n");
        }
    }
    else if (tipo == 2) {
        FILE *adcConsulta = fopen("dadosConsulta.txt", "r");
        FILE *temporario = fopen("temporario.txt", "w");

        if (adcConsulta == NULL && temporario == NULL) {
            printf("Erro ao abrir arquivo.\n");
            return;
        }

        char linha[200];
        Consulta temp;
        while (fgets(linha, sizeof(linha), adcConsulta) && 
        sscanf(linha, "Nome: %[^|]|ID consulta: %d|Horario: %d:%d|Medico: %[^\n]",
            temp.nomePaciente, &temp.idConsulta, &temp.horaH, &temp.horaM, temp.medico) == 5) {
                if (temp.idConsulta != c.idConsulta) {
                    fprintf(temporario, "Nome: %s|ID consulta: %d|Horario: %02d:%02d|Medico: %s\n",
                            temp.nomePaciente, temp.idConsulta, temp.horaH, temp.horaM, temp.medico);
                }
        }
        fclose(adcConsulta);
        fclose(temporario);

        remove("dadosConsulta.txt");
        rename("temporario.txt", "dadosConsulta.txt");

        printf("Consulta removida com sucesso.\n");
    }
}

void menuConsulta(FilaConsulta *fila) {
    int idConsulta, opc;
    Consulta at;

    printf("1 - Cadastrar consulta\n2 - Restaurar ultima consulta\n3 - Atender paciente\n");
    scanf("%d", &opc);
    getchar(); 

    switch (opc) {
        case 1: {
        Consulta novaConsulta;

        printf("Digite o ID do paciente que deseja cadastrar a consulta: ");
        scanf("%d", &idConsulta);
        getchar(); 

        Paciente *pBusca = buscaPaciente(idConsulta, 0);

        if (pBusca != NULL) {
        novaConsulta.idConsulta = totalConsulta + 1;
        novaConsulta.idPaciente = idConsulta;
        strcpy(novaConsulta.nomePaciente, pBusca->nome);

        printf("Paciente %d\nNome: %s\n", pBusca->id, pBusca->nome);
        printf("Informe o horario que deseja marcar a consulta (HH MM): ");
        scanf("%d%d", &novaConsulta.horaH, &novaConsulta.horaM);
        getchar(); 

        printf("Informe o nome do medico: ");
        fgets(novaConsulta.medico, sizeof(novaConsulta.medico), stdin);
        novaConsulta.medico[strcspn(novaConsulta.medico, "\n")] = '\0';  

        arquivo(1, novaConsulta);              
        filaEspera(fila, novaConsulta);        
        empilharConsulta(&pilha, novaConsulta); 

        totalConsulta++;
        printf("\nConsulta cadastrada com sucesso.\n");
        } else {
        printf("Não encontramos o paciente.\n");
        }

        break;
        }

        case 2:
            restaurarConsulta(&pilha);
            break;

        case 3:
            at = atenderPaciente(fila);
            if (strlen(at.nomePaciente) > 0) {
                printf("\nPaciente %s, dirigir-se a sala de atendimento.\n", at.nomePaciente);
                printf("Paciente atendido com sucesso.\n");
                arquivo(2, at); 
            }
            break;

        default:
            printf("Opção inválida.\n");
            break;
    }

}

void listarConsultas() {
    printf("Lista de consultas:\n");
    FILE *arquivo = fopen("dadosConsulta.txt", "r");
    if (arquivo != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), arquivo)) {
            int idConsulta, h, hm;
            char nome[100], medico[50];

            if (sscanf(linha, "Nome: %[^|]|ID consulta: %d|Horario: %d:%d|Medico: %[^\n]",
                       nome, &idConsulta, &h, &hm, medico) == 5) {
                printf("Nome: %s | ID consulta: %d | Horario: %02d:%02d | Medico: %s\n\n",
                       nome, idConsulta, h, hm, medico);
            } else {
                printf("Erro ao ler linha: %s\n", linha);
            }
        }
        fclose(arquivo);
    } else {
        printf("Nenhuma consulta encontrada.\n");
    }
}

void editarConsulta() {
    int idEditar, i;
    printf("\nDigite o id do paciente que deseja editar a consulta: ");
    scanf("%d", &idEditar);

    Paciente *pBusca = buscaPaciente(idEditar,0);
    if (pBusca != NULL) {
        for (i = 0; i < totalConsulta; i++) {
            if (idEditar == consultas[i]->idPaciente) {
                printf("O horário da consulta do(a) %s está marcada para %02d:%02d.\n", pBusca->nome, consultas[i]->horaH, consultas[i]->horaM); 
                printf("Digite o novo horário: ");   
                scanf("%d %d", &consultas[i]->horaH, &consultas[i]->horaM);
                printf("O nome do médico cadastrado na consulta é %s.\n Digite o novo nome do médico: ", consultas[i]->medico);
                getchar();
                fgets(consultas[i]->medico, sizeof(consultas[i]->medico), stdin);
                consultas[i]->medico[strcspn(consultas[i]->medico, "\n")] = 0;

                
                arquivo(2, *consultas[i]);  
                arquivo(1, *consultas[i]);
                printf("\nConsulta editada.\n");
            break;
            }
        }    
    } else {
        printf("ID não encontrado.\n");
    }
    
    limparTela();
}

void cancelarConsulta() {
    int i, idCancelar;
    Consulta *pA, *pP;
    iniciarPilha(&pilha);

    printf("\nDigite o ID do paciente que deseja cancelar a consulta: ");
    scanf("%d", &idCancelar);

    for (i = 0; i < totalConsulta; i++) {
        if (consultas[i]->idPaciente == idCancelar) {
            empilharConsulta(&pilha, *consultas[i]);
            arquivo(2, *consultas[i]);
            
            pA = consultas[i];
            pP = pA + 1;

            for (; i < totalConsulta - 1; i++) {
                *pA = *pP;
                 pA++;
                 pP++;
            }
            totalConsulta--;
            *consultas = realloc(consultas, totalConsulta * sizeof(Consulta));
            if (consultas == NULL && totalConsulta > 0) {
                printf("Erro ao realocar memória.\n");
            }
            printf("\nConsulta cancelada com sucesso!\n"); 
            break; 
        }     
    } 
    limparTela();
}


int main() {
    int escolha;
    int sair = 0;
    FilaConsulta fila;
    iniciarFila(&fila);
    carregarPacientesDoArquivo();
    carregarConsultasDoArquivo();
    
    if (totalUsuarios == 0) {
        registrarUsuario();
    }

    printf("Sistema de Login\n");
    if (!autenticarUsuario()) {
        printf("Falha na autenticação. Saindo...\n");
        return 0;
    }
    
    while (!sair) {
        printf("\nMenu de opções:\n");
        printf("1. Adicionar paciente\n");
        printf("2. Listar pacientes\n");
        printf("3. Editar paciente\n");
        printf("4. Remover paciente\n");
        printf("5. Menu consulta\n");
        printf("6. Listar consultas\n");
        printf("7. Editar consulta\n");
        printf("8. Cancelar consulta\n");
        printf("9. Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &escolha);

        switch (escolha) {
            case 1: adicionarPaciente(); break;
            case 2: listarPacientes(); break;
            case 3: editarPaciente(); break;
            case 4: removerPaciente(); break;
            case 5: menuConsulta(&fila);break;
            case 6: listarConsultas(); break;
            case 7: editarConsulta(); break;
            case 8: cancelarConsulta(); break;
            case 9: sair = 1; break;
            default: 
                printf("Opção inválida. Tente novamente.\n");
                break;
        }
    }
    printf("Saindo...\n");
    return 0;
}
