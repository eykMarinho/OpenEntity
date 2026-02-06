/*
    OpenEntity - Sistema de Entidade-Componente (ECS)
    Autor: Marinho (Refatorado para Sparse Set)
    
    Descrição:
    Arquitetura modular baseada em componentes utilizando o padrão Sparse Set 
    para iteração O(N) em memória contígua e verificação O(1).
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
    g_ComponentData[MAX_ECS_COMPONENTS][MAX_ECS_ENTITIES],
    
    // Sparse Set Core
    g_DenseCount[MAX_ECS_COMPONENTS],
    g_DenseList[MAX_ECS_COMPONENTS][MAX_ECS_ENTITIES],
    g_SparseArray[MAX_ECS_COMPONENTS][MAX_ECS_ENTITIES],
    bool:g_EntityHasComponent[MAX_ECS_ENTITIES][MAX_ECS_COMPONENTS],

    g_EntityActive[MAX_ECS_ENTITIES],
    g_EntityType[MAX_ECS_ENTITIES];

// Macros de Iteração e Acesso
#define ECS_With(%1) _ecs_iter:%1
#define ECS_Value(%1,%2) (g_ComponentData[%2][%1])

// Iterador Otimizado (Sparse Set)
#define foreach_ecs(%1:%2) \
    for (new _idx_%1 = 0, %1; _idx_%1 < g_DenseCount[%2] && (%1 = g_DenseList[%2][_idx_%1]) != -1; _idx_%1++)

// API

stock ECS_RegisterComponent(const name[]) {
    if (g_ComponentCount >= MAX_ECS_COMPONENTS) {
        printf("[ECS] Erro: Maximo de componentes atingido (%d)", MAX_ECS_COMPONENTS);
        return -1;
    }
    new id = g_ComponentCount++;
    ECS_Strcpy(g_ComponentNames[id], name);
    g_DenseCount[id] = 0;
    return id;
}

stock ECS_CreateEntity() {
    #if MAX_ECS_ENTITIES > 5000
        new start_index = 5000;
    #else
        new start_index = MAX_ECS_ENTITIES / 2;
    #endif

    for(new i = start_index; i < MAX_ECS_ENTITIES; i++) {
        if (!g_EntityActive[i]) {
            g_EntityActive[i] = true;
            g_EntityType[i] = ENTITY_TYPE_CUSTOM;
            return i;
        }
    }
    return INVALID_ENTITY;
}

stock ECS_DestroyEntity(entity) {
    if (entity < 0 || entity >= MAX_ECS_ENTITIES) return 0;
    
    for(new c = 0; c < g_ComponentCount; c++) {
        if (g_EntityHasComponent[entity][c]) {
            ECS_RemoveComponent(entity, c);
        }
    }
    
    g_EntityActive[entity] = false;
    g_EntityType[entity] = ENTITY_TYPE_NONE;
    return 1;
}

stock ECS_AddComponent(entity, component, {_,Float}:value = 0) {
    if (entity < 0 || entity >= MAX_ECS_ENTITIES) return 0;
    if (component < 0 || component >= g_ComponentCount) return 0;
    
    if (g_EntityHasComponent[entity][component]) {
        g_ComponentData[component][entity] = _:value;
        return 1;
    }

    g_EntityActive[entity] = true; 
    g_EntityHasComponent[entity][component] = true;
    g_ComponentData[component][entity] = _:value;

    // Sparse Set Add
    new count = g_DenseCount[component];
    g_DenseList[component][count] = entity;
    g_SparseArray[component][entity] = count;
    g_DenseCount[component]++;
    
    return 1;
}

stock ECS_RemoveComponent(entity, component) {
    if (!g_EntityHasComponent[entity][component]) return 0;
    
    // Sparse Set Remove (Swap & Pop)
    new index = g_SparseArray[component][entity];
    new lastIndex = g_DenseCount[component] - 1;
    new lastEntity = g_DenseList[component][lastIndex];

    g_DenseList[component][index] = lastEntity;
    g_SparseArray[component][lastEntity] = index;

    g_DenseList[component][lastIndex] = INVALID_ENTITY;
    g_DenseCount[component]--;

    g_EntityHasComponent[entity][component] = false;
    g_ComponentData[component][entity] = 0;
    
    return 1;
}

stock ECS_Get(entity, component) {
    if (!g_EntityHasComponent[entity][component]) return 0;
    return g_ComponentData[component][entity];
}

stock ECS_Set(entity, component, {_,Float}:value) {
    if (!g_EntityHasComponent[entity][component]) return 0;
    g_ComponentData[component][entity] = _:value;
    return 1;
}

stock bool:ECS_Has(entity, component) {
    if (entity < 0 || entity >= MAX_ECS_ENTITIES) return false;
    return g_EntityHasComponent[entity][component];
}

stock ECS_Dump() {
    printf("--- ECS Dump (Sparse Set) ---");
    printf("Components: %d", g_ComponentCount);
    for(new c = 0; c < g_ComponentCount; c++) {
        printf("Componente '%s' (Count: %d):", g_ComponentNames[c], g_DenseCount[c]);
        for(new i = 0; i < g_DenseCount[c]; i++) {
            new entity = g_DenseList[c][i];
            printf("  [%d] Entity ID: %d | Val: %d", i, entity, g_ComponentData[c][entity]);
        }
    }
    printf("--- End Dump ---");
}

static stock ECS_Strcpy(dest[], const source[], len = sizeof(dest)) {
    dest[0] = '\0';
    strcat(dest, source, len);
}
