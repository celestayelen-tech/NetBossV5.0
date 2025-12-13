# NetBoss v5.0 - Professional Network Audit Tool

**NetBoss** es una herramienta de auditoría de red de bajo nivel y reconocimiento activo, escrita estrictamente en C puro utilizando la API nativa de Windows (Win32/Winsock2). Diseñada para entornos DevSecOps y Análisis Forense, opera sin dependencias externas, garantizando portabilidad total y una huella de memoria mínima.

A diferencia de las herramientas estándar que confían en las abstracciones del sistema operativo, NetBoss interactúa directamente con el stack de red para validar conectividad, realizar huellas digitales de infraestructura y detectar configuraciones de red anómalas.

---

# Capacidades Técnicas
Reconocimiento de Red (Low-Level)
Validación de Conectividad por Interfaz: Realiza Socket Binding forzado para verificar la salida a internet de cada adaptador individualmente, eliminando falsos positivos de interfaces virtuales (Hyper-V, VirtualBox).

Latencia Real (ICMP Nativo): Implementación directa de IcmpSendEcho para mediciones de latencia con precisión de milisegundos, evitando la sobrecarga de subprocesos del sistema.

Enumeración de Interfaces: Uso avanzado de IPHLPAPI para mapeo completo de adaptadores, puertas de enlace y direcciones unicast.

# Inteligencia & OSINT
Detección Heurística de CGNAT: Análisis comparativo entre IP WAN y LAN para detectar Carrier-Grade NAT (RFC 6598) y doble natting.

Reconocimiento de Identidad (OSINT): Geolocalización de IP pública y detección de ISP/ASN mediante consultas API sigilosas.

Evasión (Stealth): Rotación aleatoria de cabeceras User-Agent en cada petición para evadir bloqueos simples basados en firmas.

# Auditoría de Seguridad (Active Recon)
Gateway Fingerprinting: Escaneo activo de puertos administrativos (80/443) en la Puerta de Enlace predeterminada para identificar routers vulnerables o paneles de administración expuestos.

Integridad de Binario: Firma digital interna (AUTHOR_SIGNATURE) inmutable compilada en el segmento de datos para análisis forense y verificación de autoría.

# Arquitectura & UX
GUI Nativa Win32: Interfaz gráfica ligera sin frameworks (sin Electron/Qt), renderizada directamente por GDI32.

Multithreading: Motor de escaneo asíncrono (_beginthread) para evitar congelamientos de UI durante operaciones de red intensivas.

Forensics Logging: Generación de reportes de auditoría en tiempo real.

# Compilación (Build from Source)
El proyecto requiere MinGW-w64 (GCC) y las cabeceras de Windows SDK.

1. Compilar Recursos (Icono y Manifiesto de Estilo):

Bash

windres resource.rc -o resource.o

2. Compilar Binario (Linkeo Estático):

Bash

gcc main.c resource.o -o NetBoss.exe -liphlpapi -lws2_32 -lgdi32 -mwindows

# Flags explicados:

-liphlpapi: API de ayuda IP (Adaptadores).

-lws2_32: Windows Sockets 2.0.

-lgdi32: Graphics Device Interface (Para la GUI).

-mwindows: Oculta la consola de comandos al ejecutar.

🔐 Verificación de Integridad (GPG)
Todos los releases oficiales están firmados criptográficamente. Para verificar que su copia de NetBoss.exe no ha sido manipulada o infectada:

Descargue NetBoss.exe y la firma NetBoss.exe.sig.

Importe la clave pública del desarrollador (ID: 4BC71F714221FA4608B5E4B315967B1BE4C59B69)

# Verifique con GnuPG:

Bash

gpg --verify NetBoss.exe.sig NetBoss.exe
Salida esperada: Firma correcta de "Celeste Ayelen <celestayelen@gmail.com>"

⚠️ Advertencia Legal
Uso exclusivo para auditorías propias o entornos de prueba autorizados. El uso de esta herramienta contra redes de terceros sin consentimiento explícito y por escrito puede violar leyes locales e internacionales de ciberseguridad. La autora no se hace responsable del mal uso de este software..

⚖️ Licencia y Autoría
Copyright (C) 2025 Celeste Ayelen.

Este programa es software libre: puede redistribuirlo y/o modificarlo bajo los términos de la Licencia Pública General GNU (GPL) publicada por la Free Software Foundation, versión 3 de la Licencia.