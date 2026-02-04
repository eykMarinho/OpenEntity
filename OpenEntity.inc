/*
    OpenEntity - Sistema de Entidade-Componente (ECS)
    Autor: Marinho
    
    Descrição:
    Permite que desenvolvedores abandonem enums gigantescos e código espaguete 
    em favor de uma arquitetura modular baseada em componentes.
    
    Funcionalidades:
    - Entidades Dinâmicas (Jogadores, Veículos, Objetos Customizados)
    - Anexação/Remoção Rápida de Componentes
    - Iteração O(N) sobre entidades com componentes específicos
    - Wrappers com segurança de tipo
    
    Uso:
    
    new COMP_SAUDE;
    
    public OnGameModeInit() {
        COMP_SAUDE = ECS_RegisterComponent("Saude");
    }
    
    ECS_AddComponent(entity_id, COMP_SAUDE, 100);
    
    foreach_ecs(i : COMP_SAUDE) {
        printf("Entidade %d tem saude %d", i, ECS_Value(i, COMP_SAUDE));
    }
*/

#if defined _open_entity_included
    #endinput
#endif
#define _open_entity_included


// Configuração
#if !defined MAX_ECS_ENTITIES
    #define MAX_ECS_ENTITIES 10000
#endif

#if !defined MAX_ECS_COMPONENTS
    #define MAX_ECS_COMPONENTS 64
#endif

// Tipos
#define INVALID_ENTITY (-1)
#define ENTITY_TYPE_NONE (0)
#define ENTITY_TYPE_PLAYER (1)
#define ENTITY_TYPE_VEHICLE (2)
#define ENTITY_TYPE_CUSTOM (3)

// Armazenamento Interno (Público para acesso via macro de alta performance)
new 
    g_ComponentCount = 0,
    g_ComponentNames[MAX_ECS_COMPONENTS][32],
    // Armazenamento de Dados (Inteiro Simples por enquanto, pode ser cast para Float)
    g_ComponentData[MAX_ECS_COMPONENTS][MAX_ECS_ENTITIES],
    // Bitmask para verificações rápidas de "Tem Componente"
    bool:g_EntityHasComponent[MAX_ECS_ENTITIES][MAX_ECS_COMPONENTS],
    // Lista Encadeada para Iteração Rápida
    g_FirstEntity[MAX_ECS_COMPONENTS] = {-1, ...},
    g_NextEntity[MAX_ECS_COMPONENTS][MAX_ECS_ENTITIES] = {-1, ...},
    g_PrevEntity[MAX_ECS_COMPONENTS][MAX_ECS_ENTITIES] = {-1, ...},
    // Gerenciamento de Entidade
    bool:g_EntityActive[MAX_ECS_ENTITIES],
    g_EntityType[MAX_ECS_ENTITIES];

// Mágica do Iterador para "foreach(new i : ECS_With(comp))"
// Utilizamos a interface de iterador customizada do YSI ou um loop de macro simplificado
#define ECS_With(%1) _ecs_iter:%1

// Acesso direto para alta performance (Sem checagem de segurança)
#define ECS_Value(%1,%2) (g_ComponentData[%2][%1])

// Implementação de iterador customizado otimizado
// Acesso direto aos arrays para evitar overhead de chamada de função
#define foreach_ecs(%1:%2) \
    for (new %1 = g_FirstEntity[%2]; %1 != -1; %1 = g_NextEntity[%2][%1])

// API

/**
 * Registra um novo componente. Deve ser chamado em OnGameModeInit.
 * @param name Nome do componente para depuração
 * @return ID do Componente
 */
stock ECS_RegisterComponent(const name[]) {
    if (g_ComponentCount >= MAX_ECS_COMPONENTS) {
        printf("[ECS] Erro: Maximo de componentes atingido (%d)", MAX_ECS_COMPONENTS);
        return -1;
    }
    new id = g_ComponentCount++;
    ECS_Strcpy(g_ComponentNames[id], name);
    // Inicializar lista
    g_FirstEntity[id] = -1;
    return id;
}

/**
 * Cria uma nova entidade customizada.
 * @return ID da Entidade
 */
stock ECS_CreateEntity() {
    // Encontrar slot livre começando de 5000 (reservado para entidades customizadas)
    // 0-999: Jogadores
    // 1000-2999: Veículos
    // 5000+: Customizados
    for(new i = 5000; i < MAX_ECS_ENTITIES; i++) {
        if (!g_EntityActive[i]) {
            g_EntityActive[i] = true;
            g_EntityType[i] = ENTITY_TYPE_CUSTOM;
            return i;
        }
    }
    return INVALID_ENTITY;
}

/**
 * Destrói uma entidade e remove todos os seus componentes.
 */
stock ECS_DestroyEntity(entity) {
    if (entity < 0 || entity >= MAX_ECS_ENTITIES) return 0;
    
    // Remover todos os componentes
    for(new c = 0; c < g_ComponentCount; c++) {
        if (g_EntityHasComponent[entity][c]) {
            ECS_RemoveComponent(entity, c);
        }
    }
    
    g_EntityActive[entity] = false;
    g_EntityType[entity] = ENTITY_TYPE_NONE;
    return 1;
}

/**
 * Adiciona um componente a uma entidade.
 * @param entity O ID da entidade
 * @param component O ID do componente
 * @param value Valor inicial (int/float/bool)
 */
stock ECS_AddComponent(entity, component, {_,Float}:value = 0) {
    if (entity < 0 || entity >= MAX_ECS_ENTITIES) return 0;
    if (component < 0 || component >= g_ComponentCount) return 0;
    
    if (g_EntityHasComponent[entity][component]) {
        // Apenas atualizar valor se já existe
        g_ComponentData[component][entity] = _:value;
        return 1;
    }

    // Marcar como ativo
    g_EntityActive[entity] = true; // Implicitamente ativa a entidade se for jogador/veículo
    g_EntityHasComponent[entity][component] = true;
    g_ComponentData[component][entity] = _:value;

    // Adicionar à Lista Encadeada (Inserir no Início)
    new oldFirst = g_FirstEntity[component];
    g_FirstEntity[component] = entity;
    g_NextEntity[component][entity] = oldFirst;
    g_PrevEntity[component][entity] = -1;
    
    if (oldFirst != -1) {
        g_PrevEntity[component][oldFirst] = entity;
    }
    
    return 1;
}

/**
 * Remove um componente de uma entidade.
 */
stock ECS_RemoveComponent(entity, component) {
    if (!g_EntityHasComponent[entity][component]) return 0;
    
    // Remover da Lista Encadeada
    new prev = g_PrevEntity[component][entity];
    new next = g_NextEntity[component][entity];
    
    if (prev != -1) {
        g_NextEntity[component][prev] = next;
    } else {
        g_FirstEntity[component] = next;
    }
    
    if (next != -1) {
        g_PrevEntity[component][next] = prev;
    }
    
    g_EntityHasComponent[entity][component] = false;
    g_ComponentData[component][entity] = 0;
    
    return 1;
}

/**
 * Obtém o valor do componente.
 */
stock ECS_Get(entity, component) {
    if (!g_EntityHasComponent[entity][component]) return 0;
    return g_ComponentData[component][entity];
}

/**
 * Define o valor do componente.
 */
stock ECS_Set(entity, component, {_,Float}:value) {
    if (!g_EntityHasComponent[entity][component]) return 0;
    g_ComponentData[component][entity] = _:value;
    return 1;
}

/**
 * Verifica se a entidade possui o componente.
 */
stock bool:ECS_Has(entity, component) {
    if (entity < 0 || entity >= MAX_ECS_ENTITIES) return false;
    return g_EntityHasComponent[entity][component];
}

// Auxiliares de Iteração Interna
stock ECS_GetFirst(component) {
    if (component < 0 || component >= g_ComponentCount) return -1;
    return g_FirstEntity[component];
}

stock ECS_GetNext(component, current) {
    return g_NextEntity[component][current];
}

/**
 * Despeja todas as entidades e componentes ativos no log do servidor.
 * Útil para depuração.
 */
stock ECS_Dump() {
    printf("--- ECS Dump ---");
    printf("Components: %d", g_ComponentCount);
    for(new i = 0; i < MAX_ECS_ENTITIES; i++) {
        if (g_EntityActive[i]) {
            printf("Entity %d (Type: %d):", i, g_EntityType[i]);
            for(new c = 0; c < g_ComponentCount; c++) {
                if (g_EntityHasComponent[i][c]) {
                    printf("  - %s: %d", g_ComponentNames[c], g_ComponentData[c][i]);
                }
            }
        }
    }
    printf("--- End Dump ---");
}

// Auxiliares para cópia de strings
static stock ECS_Strcpy(dest[], const source[], len = sizeof(dest)) {
    dest[0] = '\0';
    strcat(dest, source, len);
}
