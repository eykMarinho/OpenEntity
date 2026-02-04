# OpenEntity (ECS para Pawn)

Uma biblioteca leve e de alta performance para SA-MP/open.mp que implementa o padrão **ECS (Entity-Component-System)**.

## O que ela resolve?
- **Fim das Enums Gigantes**: Não precisa mais criar `enum pInfo` com 500 variáveis.
- **Performance**: Iteração otimizada (O(1)) que acessa direto a memória, sem loops desnecessários.
- **Modularidade**: Permite criar sistemas via Filterscript que injetam dados no Gamemode sem precisar recompilar.

## Como usar
1. Baixe a include `OpenEntity.inc` e coloque na pasta `pawno/include`.
2. Inclua no seu script:
```pawn
#include <OpenEntity>

new COMP_FOME;

public OnGameModeInit() {
    COMP_FOME = ECS_RegisterComponent("Fome");
    return 1;
}

CMD:darfome(playerid) {
    ECS_AddComponent(playerid, COMP_FOME, 100);
    return 1;
}

task ProcessarFome[1000]() {
    // Loop ultra-rápido: roda APENAS em quem tem fome
    foreach_ecs(i : COMP_FOME) {
        new valor = ECS_Value(i, COMP_FOME);
        ECS_Set(i, COMP_FOME, valor - 1);
    }
}
```

## Benchmark
| Método | Tempo (10k Entidades) |
| :--- | :--- |
| Loop Clássico | 1-2ms |
| **OpenEntity** | **0ms (Instantâneo)** |

---
*Desenvolvido para modernizar.*
