# OpenEntity (ECS & Sparse Set para Pawn)

Uma biblioteca de alta performance para SA-MP/open.mp que implementa o padrão **ECS (Entity-Component-System)** utilizando a arquitetura de dados **Sparse Set**.

## 🚀 O que ela resolve?
- **Performance Extrema**: Utiliza **Sparse Sets** para garantir iteração em memória contígua (Cache Friendly). Diferente de loops tradicionais que checam todas as slots (`MAX_PLAYERS`), aqui o loop roda **apenas** nas entidades que possuem o componente.
- **Fim das "God Enums"**: Abandone a prática de criar `enum pInfo` com 500 variáveis que desperdiçam memória.
- **Modularidade Real**: Crie sistemas isolados (ex: via Filterscript) que injetam dados e comportamentos no Gamemode sem conflitos e sem recompilar o código base.

## 🛠️ Como usar
1. Baixe a include `OpenEntity.inc` e coloque na pasta `pawno/include`.
2. Inclua no seu script:

```pawn
#include <OpenEntity>

new COMP_FOME;

public OnGameModeInit() {
    // Registra o componente e recebe um ID único
    COMP_FOME = ECS_RegisterComponent("Fome");
    return 1;
}

CMD:darfome(playerid) {
    // Adiciona o componente ao jogador com valor inicial 100
    ECS_AddComponent(playerid, COMP_FOME, 100);
    return 1;
}

task ProcessarFome[1000]() {
    // Loop Otimizado (Sparse Set): 
    // Percorre um array denso e contíguo. Se tiver 10k players mas só 5 com fome, 
    // o loop rodará apenas 5 vezes.
    
    new valor;
    foreach_ecs(i : COMP_FOME) {
        valor = ECS_Value(i, COMP_FOME);
        
        if(valor > 0) {
            ECS_Set(i, COMP_FOME, valor - 1);
        } else {
            // Opcional: Remover componente se não for mais necessário
            ECS_RemoveComponent(i, COMP_FOME); 
        }
    }
}
```

## 📊 Benchmark (Iteração)
| Método | Complexidade | Tempo (10k Entidades) |
| :--- | :--- | :--- |
| Loop Clássico (Iterar Todos) | O(N_Total) | 1-2ms |
| **OpenEntity (Sparse Set)** | **O(N_Active)** | **~0ms (Desprezível)** |

> *A iteração da OpenEntity é linear e acessa a memória sequencialmente, permitindo que a CPU utilize pré-carregamento (prefetching) de cache de forma eficiente.*

---
*Desenvolvido para modernizar o desenvolvimento em Pawn.*
