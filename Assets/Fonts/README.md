# Font Awesome Integration - Instructions de téléchargement

## Téléchargement de Font Awesome 6

Pour utiliser les icônes Font Awesome dans l'éditeur Blueprint, vous devez télécharger la police Font Awesome.

### Étape 1 : Télécharger Font Awesome

1. Visitez : https://fontawesome.com/download
2. Téléchargez **Font Awesome Free for Desktop** (version gratuite)
3. Extrayez l'archive ZIP

### Étape 2 : Copier le fichier de police

1. Dans l'archive extraite, naviguez vers : `fontawesome-free-x.x.x-desktop/otfs/`
2. Copiez le fichier **Font Awesome 6 Free-Solid-900.otf**
3. Collez-le dans : `Olympe-Engine/Assets/Fonts/`
4. Renommez le fichier en : **fa-solid-900.otf**

### Structure finale

```
Olympe-Engine/
├── Assets/
│   └── Fonts/
│       └── fa-solid-900.otf  ← Fichier Font Awesome
├── Source/
│   └── Core/
│       ├── IconsFontAwesome6.h
│       ├── FontManager.h
│       └── FontManager.cpp
```

### Alternative : Téléchargement direct

Si le site officiel n'est pas accessible, vous pouvez également obtenir Font Awesome depuis :
- GitHub : https://github.com/FortAwesome/Font-Awesome/releases
- Cherchez la dernière release et téléchargez `fontawesome-free-x.x.x-desktop.zip`

### Vérification

Une fois le fichier copié, l'éditeur chargera automatiquement la police au démarrage.
Vous verrez un message dans les logs :
```
[FontManager] Font Awesome loaded successfully from: Assets/Fonts/fa-solid-900.otf
```

## License

Font Awesome Free est sous licence **SIL OFL 1.1 License** :
- ✅ Utilisation commerciale autorisée
- ✅ Modification autorisée
- ✅ Distribution autorisée
- ✅ Utilisation privée autorisée

Pour plus d'informations : https://fontawesome.com/license/free
